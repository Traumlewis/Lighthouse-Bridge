# Protocol and dependencies

This is a new C++ implementation; it does not embed the Flutter application.
The power protocol was checked against:

- [Lighthouse PM, Lighthouse V2 device](https://github.com/jeroen1602/lighthouse_pm/blob/1b1433d124ab3852c4aad62a8265a96a4997d7d0/lighthouse_provider/lighthouse_providers/lib/src/lighthouse_v2/device/lighthouse_v2_device.dart):
  service `00001523-1212-efde-1523-785feabcd124`, power characteristic
  `00001525-1212-efde-1523-785feabcd124`, Write Without Response:
  ON `01`, SLEEP `00`. STANDBY `02` is displayed only when read, never sent.

Pinned build dependencies:

| Package | Version | Source / license |
| --- | --- | --- |
| PlatformIO espressif32 | 6.9.0 | [Platform](https://github.com/platformio/platform-espressif32), Apache-2.0 |
| Arduino ESP32 | 2.0.17, selected by the platform | [Espressif](https://github.com/espressif/arduino-esp32), LGPL-2.1 |
| NimBLE-Arduino | 1.4.3 | [h2zero](https://github.com/h2zero/NimBLE-Arduino), Apache-2.0 and embedded third-party components |
| WiFiManager | 2.0.17 | [tzapu](https://github.com/tzapu/WiFiManager), MIT |
| ArduinoJson | 6.21.5 | [bblanchon](https://github.com/bblanchon/ArduinoJson), MIT |
| Espalexa | 2.7.0 / commit `40bf53a0c2e73c25d66da9a66a70307f1d304314` | [Aircoookie](https://github.com/Aircoookie/Espalexa), MIT |

License/notice files shipped with the compiled dependencies are included in the
firmware ZIP under `third-party-licenses/`. Refer to the respective source
packages for additional notices; this table does not replace those notices.

The Alexa implementation uses the unmodified Espalexa library. Its stated Echo
compatibility is a project goal, not evidence of testing with an Echo Dot 5
running current firmware.
