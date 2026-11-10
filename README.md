# Braids on Raspberry Pi Pico RP2040

A standalone synthesizer using the Mutable Instruments Braids sound engine, an
original Raspberry Pi Pico, a PCM5102 I2S DAC, a 128x32 SSD1306 OLED, and a rotary encoder.

**Version 0.1.0 is the first working hardware baseline.** Audio, the OLED menu,
and encoder rotation/button operation have been confirmed on the assembled
prototype by the project owner. The baseline runs at 133 MHz with 48 kHz,
16-bit audio, duplicating the mono signal to both DAC channels.

## Hardware and wiring

- Original Raspberry Pi Pico / Pico H: RP2040, 2 MB flash.
- Purple PCM5102 breakout with onboard regulators and a 5 V VIN input.
- 0.91-inch, 128x32, I2C SSD1306 OLED.
- Encoder breakout with CLK, DT, SW, +, and GND pins.
- USB data cable, jumper wires, and powered speakers or an audio line input.

Disconnect USB power before changing wiring. The physical pin numbers below
refer to the **original Pico's 40-pin layout**, not a full-size Raspberry Pi.

| Component | Module pin | Pico connection | Physical Pico pin |
| --- | --- | --- | --- |
| OLED | VCC / VDD | **3V3 OUT** | **36** |
| OLED | GND | GND | 38 |
| OLED | SDA | GP4 | 6 |
| OLED | SCL / SCK | GP5 | 7 |
| Encoder | + | **3V3 OUT** | **36** |
| Encoder | GND | GND | 8 |
| Encoder | CLK | GP6 | 9 |
| Encoder | DT | GP7 | 10 |
| Encoder | SW | GP8 | 11 |
| PCM5102 | VIN | **VBUS, 5 V from USB** | **40** |
| PCM5102 | GND | GND | 13 |
| PCM5102 | SCK | GND, internal PLL mode | 18 |
| PCM5102 | BCK | GP10 | 14 |
| PCM5102 | RCK / LCK / LRCK | GP11 | 15 |
| PCM5102 | DIN | GP12 | 16 |

All signal GPIOs are separate. The OLED and encoder share the 3.3 V output;
use a breadboard rail to distribute it. All ground pins are internally connected.
Power the encoder and OLED from **3.3 V**, keeping their pull-ups at GPIO-safe levels.

The DAC power connection applies to the tested breakout, matching this
[module schematic](https://macsbug.wordpress.com/wp-content/uploads/2021/02/pcm5102a_dac_schematic.pdf).
Verify supply requirements before using another breakout. A3V3 is not the 5 V
input. RCK is the channel clock, not the right analog output. SCK is grounded
for internal PLL operation. Intended configuration: FLT low, DEMP low,
XSMT high (unmuted), and FMT low (I2S); the tested board's jumpers were retained.

Connect the DAC jack to powered speakers or a line input, initially at low volume.
Headphones or passive speakers need an appropriate amplifier. The firmware
checks OLED addresses 0x3C and 0x3D automatically. Follow each module's printed
pin labels because connector order can vary.

## Setup and build on Windows

Requirements: **64-bit Windows, PowerShell, Git, and Internet access for setup**.
Arduino IDE, PlatformIO, and a system-wide Python installation are not required.

```powershell
git clone https://github.com/FedeRepic/braids_rpico_2040.git
cd braids_rpico_2040
.\scripts\Setup.ps1
.\scripts\Build.ps1 -Target BraidsControl
```

Setup downloads Arduino CLI, the Arduino-Pico core/toolchain, and libraries
inside the project. It does not modify PATH or install global Arduino libraries.
Versions and source revisions are pinned in [dependencies.json](dependencies.json),
and the CLI archive is checked against its SHA256 checksum. Existing PowerShell
execution-policy settings still apply.

Setup preserves existing libraries in `vendor/`. Build scripts regenerate local
absolute configuration paths, so the project directory can be moved.

| Build target | Purpose | UF2 output at 133 MHz |
| --- | --- | --- |
| `BraidsControl` | Full OLED/encoder synthesizer | `build/BraidsControl-133/BraidsControl.ino.uf2` |
| `BraidsAudio` | Fixed CSAW sound without controls | `build/BraidsAudio-133/BraidsAudio.ino.uf2` |
| `ToneTest` | Continuous 440 Hz sine for DAC testing | `build/ToneTest-133/ToneTest.ino.uf2` |

Select the target explicitly; the script defaults to `BraidsAudio`. To build diagnostics:

```powershell
.\scripts\Build.ps1 -Target ToneTest
.\scripts\Build.ps1 -Target BraidsAudio
```

## Flash the Pico

1. Remove any temporary RUN-to-GND jumper and disconnect USB power.
2. Hold **BOOTSEL** while connecting the Pico with a USB data cable.
3. Release BOOTSEL after the `RPI-RP2` drive appears.
4. Copy `build/BraidsControl-133/BraidsControl.ino.uf2` to that drive.
5. The Pico restarts, plays CSAW at MIDI note 60 (C4), and shows the menu.

A serial monitor is not required. If the USB drive does not appear, test the Pico
alone with a known data cable and another port. A Windows device-descriptor error
must be resolved before copying a UF2. Raspberry Pi also documents
[resetting into BOOTSEL with RUN and GND](https://www.raspberrypi.com/news/how-to-add-a-reset-button-to-your-raspberry-pi-pico/).

## Controls

The on-screen labels and setup-script messages are currently in Spanish.

- **Turn:** select a menu field.
- **Short press:** enter edit mode; `* EDITAR` appears.
- **Turn in edit mode:** adjust the value and hear the change.
- **Short press again:** return to field selection.
- **Hold for about 0.6 seconds:** send a manual Strike trigger.

| On-screen label | Function | Range / choices | Startup value |
| --- | --- | --- | --- |
| Modelo | Sound model | 47 models, special `?` omitted | CSAW |
| Nota | MIDI note | 0-127 | 60 / C4 |
| Timbre | First synthesis parameter | 0-100% | 50% |
| Color | Second synthesis parameter | 0-100% | 25% |
| Nivel | Output level | 0-100% | 25% |
| Disparo | Strike source | Automatic every 0.5 s / manual | Automatic |

Strike affects models that respond to triggering. Sustained oscillators keep
playing continuously; this version does not add a separate amplitude envelope.
**Settings reset at power-on.** Preset storage is planned.

For reversed encoder direction, swap CLK/DT with power disconnected or change
`kEncoderDirection` to `-1` in `firmware/BraidsControl/ControlLogic.h` and rebuild.
The decoder expects four quadrature transitions per detent, typical of KY-040 breakouts.

## Validation and diagnostics

```powershell
.\scripts\Check-Controls.ps1
```

The local ARM compiler evaluates the actual control logic at compile time.
Checks cover rotation in both directions, bounce, invalid transitions, long
traces from all four encoder phases, field limits, short/long presses, and
32-bit timer rollover. Additional assertions run when building `BraidsControl`.
No connected Pico or separate host compiler is needed for these checks.

Audio output and OLED/encoder operation have been confirmed on the prototype.
Model-by-model timing and long-duration stability benchmarking remain future work.

| Indicator | Meaning |
| --- | --- |
| Steady Pico LED | OLED found |
| Slow blinking LED | OLED missing; audio and encoder still operate |
| Fast blinking LED | I2S initialization failed |
| `!` on OLED | Underflow detected after the first startup second; clears on reboot |

USB serial at 115200 reports the model, maximum render time for that model,
underflow events, and OLED address. At 48 kHz, each 32-sample block has about
667 microseconds available; transmission and other work also need time.

The baseline uses **133 MHz**. Demanding models may need optimization or a
higher clock. An optional overclocked build can be produced with:

```powershell
.\scripts\Build.ps1 -Target BraidsControl -CpuMHz 250
```

Output goes to `build/BraidsControl-250/`. A 250 MHz overclock needs stability
testing on the particular Pico; it is not the validated default.

## Project layout and local dependencies

```text
firmware/
  BraidsControl/              OLED + encoder synthesizer
  BraidsAudio/                Fixed-sound diagnostic sketch
  ToneTest/                   440 Hz DAC test
  libraries/ProjectAudio/     Shared I2S configuration
scripts/                      Setup, builds, and checks
tests/                        Compile-time control tests
dependencies.json             Pinned versions and revisions
CHANGELOG.md                  Version history
THIRD_PARTY.md                Dependency credits and license references
```

Tools (`.tools/`), Arduino packages/libraries/cache (`.arduino/`), source
dependencies (`vendor/`), downloads (`.downloads/`), temporary files (`.tmp/`),
and compiled outputs (`build/`) are ignored by Git. Setup/build scripts recreate
them from the tracked files. Firmware binaries and machine-specific settings
are also excluded.

These generated folders can be removed when no tools are running, provided
`vendor/` has no local changes worth preserving. Run Setup and rebuild to restore
them. Keep the firmware, scripts, tests, documentation, and configuration files.

## Implementation and next steps

Core 0 renders Braids and transmits I2S audio. Core 1 handles the OLED and menu;
encoder edges are captured by interrupts. Audio takes synchronized settings
snapshots without waiting for the UI. The OLED uses I2C0 on GP4/GP5.

Planned improvements include per-model performance measurements, preset storage,
menu refinement, and MIDI or external note/trigger control. Eurorack CV/gate
inputs will require appropriate signal-conditioning hardware.

## Acknowledgments

The sound engine originates from **Mutable Instruments / Emilie Gillet**.
This project uses the BRAIDS and STMLIB Arduino ports by **Mark Washeim (poetaster)**,
with inspiration from [arduinoMI](https://github.com/poetaster/arduinoMI).
Audio I/O uses [Arduino-Pico](https://github.com/earlephilhower/arduino-pico), and
the display uses Adafruit SSD1306, GFX, and BusIO.

See [THIRD_PARTY.md](THIRD_PARTY.md) for dependency sources and license references,
and the [official Pico pinout](https://datasheets.raspberrypi.com/pico/Pico-R3-A4-Pinout.pdf)
for the board's physical connections.
