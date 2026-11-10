# Protokoll und Abhängigkeiten

Das Projekt ist eine neue C++-Implementierung; die Flutter-App wird nicht
eingebettet. Das Power-Protokoll wurde anhand folgender Quelle geprüft:

- [Lighthouse PM, Lighthouse V2 Device](https://github.com/jeroen1602/lighthouse_pm/blob/1b1433d124ab3852c4aad62a8265a96a4997d7d0/lighthouse_provider/lighthouse_providers/lib/src/lighthouse_v2/device/lighthouse_v2_device.dart):
  Service `00001523-1212-efde-1523-785feabcd124`, Power-Characteristic
  `00001525-1212-efde-1523-785feabcd124`, Write Without Response:
  ON `01`, SLEEP `00`. STANDBY `02` wird ausschließlich beim Lesen angezeigt.

Festgelegte Build-Abhängigkeiten:

| Paket | Version | Quelle / Lizenz |
| --- | --- | --- |
| PlatformIO espressif32 | 6.9.0 | [Platform](https://github.com/platformio/platform-espressif32), Apache-2.0 |
| Arduino ESP32 | 2.0.17, durch Plattform festgelegt | [Espressif](https://github.com/espressif/arduino-esp32), LGPL-2.1 |
| NimBLE-Arduino | 1.4.3 | [h2zero](https://github.com/h2zero/NimBLE-Arduino), Apache-2.0 und eingebettete Drittkomponenten |
| WiFiManager | 2.0.17 | [tzapu](https://github.com/tzapu/WiFiManager), MIT |
| ArduinoJson | 6.21.5 | [bblanchon](https://github.com/bblanchon/ArduinoJson), MIT |
| Espalexa | 2.7.0 / Commit `40bf53a0c2e73c25d66da9a66a70307f1d304314` | [Aircoookie](https://github.com/Aircoookie/Espalexa), MIT |

Die Lizenzen der ausgelieferten Bibliotheken werden dem Firmware-ZIP unter
`third-party-licenses/` beigelegt. Zusätzliche rechtliche Hinweise sind in den
jeweiligen Quellpaketen enthalten; die Tabelle ersetzt diese nicht.

Die Alexa-Implementierung nutzt Espalexa als unveränderte Bibliothek. Die dort
beschriebene Echo-Unterstützung ist eine Projektabsicht, kein Testnachweis für
den Echo Dot 5 mit aktueller Firmware.
