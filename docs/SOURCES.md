# Protocol and dependencies

| Component | Pin | Source |
| --- | --- | --- |
| Arduino ESP32 | 3.3.12 | [Espressif](https://github.com/espressif/arduino-esp32/tree/3.3.12) |
| rs-matter | `43f61585a439f1a1e90924b696a6280850ef0145` | [project repository](https://github.com/project-chip/rs-matter) |
| NimBLE-Arduino | 2.5.2 | [h2zero](https://github.com/h2zero/NimBLE-Arduino) |
| WiFiManager | 2.0.17 | [tzapu](https://github.com/tzapu/WiFiManager) |
| ArduinoJson | 6.21.5 | [bblanchon](https://github.com/bblanchon/ArduinoJson) |
| ESP Web Tools | 10.4.0 | [ESPHome](https://github.com/esphome/esp-web-tools) |

License and notice files are included in the firmware ZIP. Exact Rust
dependencies are recorded in [Cargo.lock](../firmware/rust/Cargo.lock).

The Lighthouse V2 control service is
`00001523-1212-efde-1523-785feabcd124`. The power characteristic
`00001525-1212-efde-1523-785feabcd124` accepts ON (`0x01`) or Sleep (`0x00`).
Standby (`0x02`) is opt-in per station. See the upstream
[lighthouse V2 power control example](https://github.com/risa2000/lh2ctrl)
and the [NimBLE release notes](https://github.com/h2zero/NimBLE-Arduino/releases/tag/2.5.2).
