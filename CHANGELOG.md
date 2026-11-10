# Changelog

## 0.1.0 - 2026-11-10

First working hardware baseline, confirmed by the project owner with the original
Raspberry Pi Pico, PCM5102 DAC, SSD1306 128x32 OLED, and rotary encoder breakout.

- Braids sound engine with 47 selectable models.
- 48 kHz, 16-bit mono audio duplicated to both I2S DAC channels at 133 MHz CPU.
- OLED menu for model, MIDI note, Timbre, Color, level, and trigger mode.
- Encoder navigation/editing, debounced button, and long-press manual Strike.
- Automatic Strike every 0.5 seconds or manual operation.
- Separate cores for audio generation and OLED/control handling.
- Automatic OLED address detection at 0x3C/0x3D and USB serial diagnostics.
- Standalone fixed-sound Braids and 440 Hz diagnostic sketches.
- Project-local Windows setup/build scripts with pinned dependencies.
- Compile-time encoder, menu, and button checks.
- English setup, wiring, operation, and troubleshooting documentation.

Settings reset on power-up. Model-by-model timing, preset storage, and external
note/trigger inputs are planned improvements.
