#include <STMLIB.h>
#include <BRAIDS.h>
#include <ProjectAudio.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <hardware/gpio.h>
#include <hardware/sync.h>
#include <pico/mutex.h>

#include "ControlLogic.h"
#include "ControlChecks.h"
#include "ModelNames.h"

constexpr uint8_t kSdaPin = 4;
constexpr uint8_t kSclPin = 5;
constexpr uint8_t kClkPin = 6;
constexpr uint8_t kDtPin = 7;
constexpr uint8_t kSwitchPin = 8;
constexpr uint32_t kStrikeFrames = project_audio::kSampleRate / 2;
static_assert(controls::kModelCount == braids::MACRO_OSC_SHAPE_LAST_ACCESSIBLE_FROM_META + 1,
              "Selectable models must match this version of Braids");
static_assert(kStrikeFrames % project_audio::kBlockSize == 0, "Whole trigger blocks");

// Leave the full main-core stack available for the DSP engine.
bool core1_separate_stack = true;

struct AudioStats {
  uint32_t render_max_us = 0;
  uint32_t underflow_events = 0;
};

struct SharedState {
  controls::Settings settings;
  uint32_t strike_sequence = 0;
  AudioStats stats;
  bool audio_ready = false;
  uint8_t oled_address = 0;
};

// Static initialization happens before either core starts setup().
auto_init_mutex(shared_mutex);
SharedState shared;

// Core 0 owns the oscillator, I2S, and USB status output exclusively.
I2S audio(OUTPUT);
braids::MacroOscillator oscillator;
controls::Settings audio_settings;
AudioStats audio_stats;
int16_t samples[project_audio::kBlockSize] = {};
uint8_t sync_samples[project_audio::kBlockSize] = {};
uint32_t last_strike_sequence = 0;
uint32_t frames_to_strike = kStrikeFrames;
uint32_t startup_frames = 0;
uint32_t last_report_ms = 0;
uint8_t reported_oled_address = 0;
int32_t current_gain = 25 * 32767 / 100;

// Core 1 owns Wire, OLED, the encoder, and the menu exclusively.
Adafruit_SSD1306 display(128, 32, &Wire, -1, 400000, 400000);
controls::Menu menu;
controls::Button button;
controls::RotaryDecoder rotary;
volatile int32_t pending_detents = 0;  // Written by core 1's GPIO interrupt only.
bool oled_present = false;
bool screen_dirty = true;
uint32_t last_screen_ms = 0;

void setup() {
  Serial.begin(115200);
  oscillator.Init(static_cast<float>(project_audio::kSampleRate));
  oscillator.set_shape(static_cast<braids::MacroOscillatorShape>(audio_settings.model));
  oscillator.set_pitch(audio_settings.note * 128);
  oscillator.set_parameters(audio_settings.timbre * 32767 / 100,
                            audio_settings.color * 32767 / 100);
  oscillator.Strike();
  if (!project_audio::Begin(audio)) project_audio::StopWithError();

  mutex_enter_blocking(&shared_mutex);
  shared.audio_ready = true;
  mutex_exit(&shared_mutex);
}

void loop() {
  // The audio core never waits for the UI. Missing a snapshot is harmless:
  // it continues with its previous settings until the next 32-sample block.
  bool manual_strike = false;
  if (mutex_try_enter(&shared_mutex, nullptr)) {
    audio_settings = shared.settings;
    manual_strike = last_strike_sequence != shared.strike_sequence;
    last_strike_sequence = shared.strike_sequence;
    reported_oled_address = shared.oled_address;
    shared.stats = audio_stats;
    mutex_exit(&shared_mutex);
  }

  const auto model = static_cast<braids::MacroOscillatorShape>(audio_settings.model);
  // Model transitions generate their own Strike in the Braids engine.
  static uint8_t previous_model = 0;
  if (audio_settings.model != previous_model) {
    oscillator.set_shape(model);
    previous_model = audio_settings.model;
    audio_stats.render_max_us = 0;
    frames_to_strike = kStrikeFrames;
  }
  oscillator.set_pitch(audio_settings.note * 128);
  oscillator.set_parameters(audio_settings.timbre * 32767 / 100,
                            audio_settings.color * 32767 / 100);
  if (manual_strike || (audio_settings.automatic && frames_to_strike == 0)) {
    oscillator.Strike();
    frames_to_strike = kStrikeFrames;
  }
  if (frames_to_strike > 0) frames_to_strike -= project_audio::kBlockSize;

  const uint32_t start_us = micros();
  oscillator.Render(sync_samples, samples, project_audio::kBlockSize);
  const uint32_t render_us = micros() - start_us;
  if (render_us > audio_stats.render_max_us) audio_stats.render_max_us = render_us;

  const int32_t target_gain = audio_settings.level * 32767 / 100;
  for (size_t i = 0; i < project_audio::kBlockSize; ++i) {
    // Smooth level changes over a few milliseconds to avoid gain-step clicks.
    if (current_gain < target_gain) current_gain += (target_gain - current_gain > 32) ? 32 : target_gain - current_gain;
    if (current_gain > target_gain) current_gain -= (current_gain - target_gain > 32) ? 32 : current_gain - target_gain;
    const int16_t sample = static_cast<int16_t>(int32_t(samples[i]) * current_gain / 32767);
    audio.write16(sample, sample);
  }
  const bool underflow = audio.getUnderflow();
  if (startup_frames < project_audio::kSampleRate) {
    startup_frames += project_audio::kBlockSize;
  } else if (underflow) {
    ++audio_stats.underflow_events;
  }

  if (millis() - last_report_ms >= 1000) {
    last_report_ms = millis();
    if (Serial && Serial.availableForWrite() >= 128) {
      char status[128];
      const int size = snprintf(status, sizeof(status),
          "model=%u render_max_us=%lu block_us=667 underflow_events=%lu oled=0x%02X\n",
          unsigned(audio_settings.model), static_cast<unsigned long>(audio_stats.render_max_us),
          static_cast<unsigned long>(audio_stats.underflow_events), unsigned(reported_oled_address));
      if (size > 0 && size < static_cast<int>(sizeof(status))) {
        Serial.write(reinterpret_cast<const uint8_t*>(status), size);
      }
    }
  }
}

void OnEncoderEdge() {
  const uint32_t pins = gpio_get_all();
  const uint8_t state = (((pins >> kClkPin) & 1u) << 1) | ((pins >> kDtPin) & 1u);
  const int change = rotary.Update(state);
  if (change != 0) {
    const int32_t next = pending_detents + change;
    if (next >= -64 && next <= 64) pending_detents = next;
  }
}

void PublishSettings() {
  mutex_enter_blocking(&shared_mutex);
  shared.settings = menu.settings;
  shared.strike_sequence = menu.strike_sequence;
  mutex_exit(&shared_mutex);
}

void DrawScreen() {
  AudioStats stats;
  mutex_enter_blocking(&shared_mutex);
  stats = shared.stats;
  mutex_exit(&shared_mutex);

  char line[24];
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setCursor(0, 0);
  snprintf(line, sizeof(line), "%s %02u/47%s", kModelNames[menu.settings.model],
           unsigned(menu.settings.model + 1), stats.underflow_events ? " !" : "");
  display.print(line);
  constexpr const char* labels[] = {"Modelo", "Nota", "Timbre", "Color", "Nivel", "Disparo"};
  display.setCursor(0, 8);
  display.print(labels[menu.field]);
  display.print(menu.editing ? " * EDITAR" : " > ELEGIR");

  switch (menu.field) {
    case controls::Model:
      snprintf(line, sizeof(line), "%02u %s", unsigned(menu.settings.model + 1),
               kModelNames[menu.settings.model]);
      break;
    case controls::Note: {
      constexpr const char* names[] = {"DO", "DO#", "RE", "RE#", "MI", "FA", "FA#", "SOL", "SOL#", "LA", "LA#", "SI"};
      snprintf(line, sizeof(line), "%s%d %03u", names[menu.settings.note % 12],
               int(menu.settings.note / 12) - 1, unsigned(menu.settings.note));
      break;
    }
    case controls::Timbre: snprintf(line, sizeof(line), "%u%%", unsigned(menu.settings.timbre)); break;
    case controls::Color: snprintf(line, sizeof(line), "%u%%", unsigned(menu.settings.color)); break;
    case controls::Level: snprintf(line, sizeof(line), "%u%%", unsigned(menu.settings.level)); break;
    case controls::Trigger: snprintf(line, sizeof(line), "%s", menu.settings.automatic ? "AUTO 0.5s" : "MANUAL"); break;
  }
  display.setCursor(0, 16);
  display.setTextSize(2);
  display.print(line);
  // Blocking I2C runs only on core 1. Encoder edges still arrive via interrupt.
  display.display();
}

void setup1() {
  // Start peripherals only after core 0 has initialized the audio hardware.
  while (true) {
    mutex_enter_blocking(&shared_mutex);
    const bool ready = shared.audio_ready;
    mutex_exit(&shared_mutex);
    if (ready) break;
    delay(1);
  }
  pinMode(kClkPin, INPUT_PULLUP);
  pinMode(kDtPin, INPUT_PULLUP);
  pinMode(kSwitchPin, INPUT_PULLUP);
  const uint32_t pins = gpio_get_all();
  rotary = controls::RotaryDecoder((((pins >> kClkPin) & 1u) << 1) | ((pins >> kDtPin) & 1u));
  attachInterrupt(digitalPinToInterrupt(kClkPin), OnEncoderEdge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(kDtPin), OnEncoderEdge, CHANGE);

  Wire.setSDA(kSdaPin);
  Wire.setSCL(kSclPin);
  Wire.begin();
  Wire.setClock(400000);
  Wire.setTimeout(5);
  uint8_t address = 0;
  for (uint8_t candidate : {uint8_t(0x3C), uint8_t(0x3D)}) {
    Wire.beginTransmission(candidate);
    if (Wire.endTransmission() == 0) {
      address = candidate;
      break;
    }
  }
  if (address != 0) {
    oled_present = display.begin(SSD1306_SWITCHCAPVCC, address, true, false);
  }
  mutex_enter_blocking(&shared_mutex);
  shared.oled_address = oled_present ? address : 0;
  mutex_exit(&shared_mutex);
  pinMode(LED_BUILTIN, OUTPUT);
  if (oled_present) DrawScreen();
}

void loop1() {
  const uint32_t irq_state = save_and_disable_interrupts();
  const int turns = pending_detents;
  pending_detents = 0;
  restore_interrupts(irq_state);

  const uint32_t now = millis();
  const auto event = button.Update(digitalRead(kSwitchPin) == LOW, now);
  if (turns != 0 || event != controls::ButtonEvent::None) {
    menu.Turn(turns);
    menu.Press(event);
    PublishSettings();
    screen_dirty = true;
  }
  if (oled_present && (now - last_screen_ms >= 250 ||
      (screen_dirty && now - last_screen_ms >= 50))) {
    DrawScreen();
    last_screen_ms = millis();
    screen_dirty = false;
  }
  // Steady LED: OLED found; slow blink: OLED missing, audio/encoder still usable.
  digitalWrite(LED_BUILTIN, oled_present || ((now / 500) & 1u));
  delay(1);
}
