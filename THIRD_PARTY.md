# Third-party dependencies

Setup downloads dependencies locally; their source trees, tools, and binaries are
not committed to this repository. Their existing copyright and license notices
remain in the downloaded packages. Exact versions are recorded in
[dependencies.json](dependencies.json).

| Dependency | Version / revision | Source | License reference |
| --- | --- | --- | --- |
| Arduino CLI | 1.5.1 | [arduino/arduino-cli](https://github.com/arduino/arduino-cli) | [Upstream license](https://github.com/arduino/arduino-cli/blob/v1.5.1/LICENSE.txt) |
| Arduino-Pico | 6.3.2 | [earlephilhower/arduino-pico](https://github.com/earlephilhower/arduino-pico) | [Upstream licensing and credits](https://github.com/earlephilhower/arduino-pico#licensing-and-credits) |
| BRAIDS | `298a4de33be274911649f4eb151d3596cfe428c8` | [poetaster/BRAIDS](https://github.com/poetaster/BRAIDS) | [MIT license](https://github.com/poetaster/BRAIDS/blob/298a4de33be274911649f4eb151d3596cfe428c8/LICENSE) |
| STMLIB | `c6710ca78524afcef79b1a5a98b85611b0d7c2b4` | [poetaster/STMLIB](https://github.com/poetaster/STMLIB) | [MIT license](https://github.com/poetaster/STMLIB/blob/c6710ca78524afcef79b1a5a98b85611b0d7c2b4/LICENSE) |
| Adafruit SSD1306 | 2.5.17 | [adafruit/Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306) | [BSD license](https://github.com/adafruit/Adafruit_SSD1306/blob/2.5.17/license.txt) |
| Adafruit GFX Library | 1.13.0 | [adafruit/Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library) | [BSD license](https://github.com/adafruit/Adafruit-GFX-Library/blob/1.13.0/license.txt) |
| Adafruit BusIO | 1.17.4 | [adafruit/Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO) | [MIT license](https://github.com/adafruit/Adafruit_BusIO/blob/1.17.4/LICENSE) |

Braids and stmlib originate from Mutable Instruments, with source copyright notices
attributing Emilie Gillet. The Arduino library ports include additional notices
from Mark Washeim (poetaster). The project uses these library sources directly;
its application sketches were written separately from the arduinoMI example sketch.

The Arduino-Pico distribution includes the Pico SDK, compiler, and other components
with their own licenses. Refer to the notices shipped with those packages when
redistributing them.
