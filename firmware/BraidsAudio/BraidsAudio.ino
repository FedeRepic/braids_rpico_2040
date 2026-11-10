#include <STMLIB.h>
#include <BRAIDS.h>
#include <ProjectAudio.h>

// Edit these values for the first listening tests.
constexpr auto kModel = braids::MACRO_OSC_SHAPE_CSAW;
constexpr int16_t kPitch = 60 * 128;  // MIDI C4 in Braids' 1/128-semitone units.
constexpr int16_t kTimbre = 16384;    // 0..32767
constexpr int16_t kColor = 8192;      // 0..32767
constexpr uint32_t kStrikeFrames = project_audio::kSampleRate / 2;

I2S audio(OUTPUT);
// Static storage zero-initializes state that the port's Init() leaves untouched.
braids::MacroOscillator oscillator;
int16_t samples[project_audio::kBlockSize] = {};
uint8_t sync_samples[project_audio::kBlockSize] = {};
uint32_t frames_to_strike = kStrikeFrames;
uint32_t max_render_us = 0;
uint32_t underflow_events = 0;
uint32_t last_report_ms = 0;

void setup() {
  Serial.begin(115200);
  // Initialize the engine before starting I2S.
  oscillator.Init(static_cast<float>(project_audio::kSampleRate));
  oscillator.set_shape(kModel);
  oscillator.set_pitch(kPitch);
  oscillator.set_parameters(kTimbre, kColor);
  oscillator.Strike();
  if (!project_audio::Begin(audio)) {
    project_audio::StopWithError();
  }
}

void loop() {
  if (frames_to_strike == 0) {
    oscillator.Strike();  // Also allows testing percussion/plucked models.
    frames_to_strike = kStrikeFrames;
  }
  const uint32_t start_us = micros();
  oscillator.Render(sync_samples, samples, project_audio::kBlockSize);
  const uint32_t render_us = micros() - start_us;
  if (render_us > max_render_us) {
    max_render_us = render_us;
  }
  frames_to_strike -= project_audio::kBlockSize;

  for (size_t i = 0; i < project_audio::kBlockSize; ++i) {
    const int16_t sample = samples[i] / 4;  // Start at a modest level (-12 dB).
    audio.write16(sample, sample);
  }

  if (audio.getUnderflow()) {
    ++underflow_events;
  }
  // A short, non-waiting status report once a second; no USB connection needed.
  if (millis() - last_report_ms >= 1000) {
    last_report_ms = millis();
    if (Serial && Serial.availableForWrite() >= 96) {
      char status[96];
      const int count = snprintf(status, sizeof(status),
          "model=%u render_max_us=%lu block_us=667 underflow_events=%lu\n",
          static_cast<unsigned>(kModel), static_cast<unsigned long>(max_render_us),
          static_cast<unsigned long>(underflow_events));
      if (count > 0 && count < static_cast<int>(sizeof(status))) {
        Serial.write(reinterpret_cast<const uint8_t*>(status), count);
      }
    }
  }
}
