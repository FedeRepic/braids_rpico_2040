#pragma once

#include <Arduino.h>
#include <I2S.h>

namespace project_audio {
constexpr uint32_t kSampleRate = 48000;
constexpr size_t kBlockSize = 32;
constexpr uint8_t kBclkPin = 10;
constexpr uint8_t kLrclkPin = 11;  // Arduino-Pico requires BCLK + 1.
constexpr uint8_t kDataPin = 12;
static_assert(kLrclkPin == kBclkPin + 1, "LRCLK must follow BCLK");

inline bool Begin(I2S& output) {
  if (!output.setBCLK(kBclkPin) || !output.setDATA(kDataPin) ||
      !output.setBitsPerSample(16) || !output.setBuffers(8, 128)) {
    return false;
  }
  return output.begin(kSampleRate);
}

inline void StopWithError() {
  // No Serial wait: the board must also start with a USB power supply.
  pinMode(LED_BUILTIN, OUTPUT);
  while (true) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}
}  // namespace project_audio
