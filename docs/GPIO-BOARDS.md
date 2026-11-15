# rs-matter GPIO buttons and board profiles

Candidate 0.1.18 adds two optional momentary buttons. Connect each contact between
its assigned GPIO and GND. The input uses the internal 3.3-V pull-up; do not apply
external voltage. Both bindings start disabled. Sign in with your own password,
open **GPIO buttons**, choose separate GPIOs and assign **All on** / **All off**.
The two actions can be swapped. Saving does not switch any station.

| WebFlasher choice | Example button GPIOs | Flash / PSRAM profile | USB bootloader offset |
| --- | --- | --- | --- |
| ESP-WROOM-32 / ESP-32S | 25 and 26 | 4 MB / disabled | 0x1000 |
| WeAct ESP32-C3FH4 | 3 and 4 | 4 MB / none | 0x0000 |
| ESP32 C3 SuperMini | 3 and 4 | 4 MB / none | 0x0000 |
| ESP32 S3 SuperMini | 4 and 5 | 4 MB / 2 MB QSPI | 0x0000 |

Use GPIO numbers, not physical header positions. Verify the labels and exposed
pins of your exact board. The supported selection excludes boot straps, flash,
PSRAM, USB, serial and common LED conflicts. BOOT still opens setup after a
five-second hold; it is GPIO9 on C3 and GPIO0 on ESP32/S3, and cannot be assigned.

Presses are debounced for 40 ms and never repeat while held. Booting or saving
with a button pressed does not send power: release it before the next press.
All off follows each station's saved SLEEP/STANDBY preference. BLE remains serial
and measured status remains authoritative. Presses blocked during setup, OTA or
restart are discarded rather than replayed. No extra physical station read/write
is introduced merely by configuring a button.

Choose the exact board before installing. Every profile has a separate manifest,
source ZIP and OTA package; image chip IDs are checked before flashing/OTA. All
profiles retain the complete partition table and both existing OTA app slots.
The original ESP32 cannot run C3/S3 firmware. New physical boards need their own
Wi-Fi/assignments/pairing; do not copy raw NVS partitions between chips.

The S3 target uses 4 MB flash and enables 2 MB QSPI PSRAM, matching the owner's
specification. Existing internal-only protocol allocation policy remains intact;
PSRAM availability alone is not proof of additional runtime headroom. C3/S3
startup, BLE/coexistence, GPIO wiring, OTA and Alexa pairing still require physical
validation. Software build/browser tests are not hardware tests.

Pin exclusions follow Espressif's [C3 GPIO reference](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32c3/api-reference/peripherals/gpio.html)
and [S3 GPIO reference](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32s3/api-reference/peripherals/gpio.html).
