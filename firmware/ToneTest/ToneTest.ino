#include <ProjectAudio.h>
#include <math.h>

// A continuous 440 Hz sine, mono duplicated to both line outputs.
// The lookup table keeps floating-point work out of the audio loop.
I2S audio(OUTPUT);
int16_t sine_table[1024];
uint32_t phase = 0;
constexpr uint32_t kPhaseStep =
    static_cast<uint32_t>(440.0 * 4294967296.0 / project_audio::kSampleRate + 0.5);

void setup() {
  Serial.begin(115200);
  for (size_t i = 0; i < 1024; ++i) {
    sine_table[i] = static_cast<int16_t>(4096.0 * sin(6.283185307179586 * i / 1024));
  }
  if (!project_audio::Begin(audio)) {
    project_audio::StopWithError();
  }
}

void loop() {
  for (size_t i = 0; i < project_audio::kBlockSize; ++i) {
    const int16_t sample = sine_table[phase >> 22];
    phase += kPhaseStep;
    // Blocking writes provide pacing; they run here, never in an interrupt.
    audio.write16(sample, sample);
  }
}
