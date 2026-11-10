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
| NimBLE-Arduino | 2.5.2 | [h2zero](https://github.com/h2zero/NimBLE-Arduino), Apache-2.0 and embedded third-party components |
| WiFiManager | 2.0.17 | [tzapu](https://github.com/tzapu/WiFiManager), MIT |
| ArduinoJson | 6.21.5 | [bblanchon](https://github.com/bblanchon/ArduinoJson), MIT |
| Espalexa | 2.7.0 / commit `40bf53a0c2e73c25d66da9a66a70307f1d304314` | [Aircoookie](https://github.com/Aircoookie/Espalexa), MIT |

License/notice files shipped with the compiled dependencies are included in the
firmware ZIP under `third-party-licenses/`. Refer to the respective source
packages for additional notices; this table does not replace those notices.

The Alexa implementation uses unmodified Espalexa for discovery, bridge metadata
and username registration. The bridge handles light GETs and validated light
PUTs itself, with tested per-ID acknowledgements. Since 0.2.6, Hue ON can briefly
follow an accepted in-flight target under the owner-approved 30-second limit;
dashboard/API data and all reachability remain measured. Alexa discovery remains local Hue emulation; compatibility depends on the Alexa
device/network/firmware. The owner's four-station setup is the hardware evidence,
not a restriction to a specific Alexa model. OTA uses Arduino ESP32's bundled
Update library and ESP-IDF OTA partition APIs; no new updater dependency is added.

The host upgrade in 0.2.4 follows the upstream
[2.5.0 connection-establishment retry addition](https://github.com/h2zero/NimBLE-Arduino/releases/tag/2.5.0),
with the [2.5.2 fixes](https://github.com/h2zero/NimBLE-Arduino/releases/tag/2.5.2).
The maintainer also recommends connection retries for persistent 0x3e failures
in [issue 1180](https://github.com/h2zero/NimBLE-Arduino/issues/1180).
This is a software recovery mechanism, not proof of a particular physical cause.

Hue write response format: [deCONZ light-state API](https://dresden-elektronik.github.io/deconz-rest-doc/endpoints/lights/#set-light-state),
with per-property success entries such as `/lights/<id>/state/on`. The prior
fallback is visible in the pinned
[Espalexa handler](https://github.com/Aircoookie/Espalexa/blob/40bf53a0c2e73c25d66da9a66a70307f1d304314/src/Espalexa.h).

## Alexa app state compatibility investigation

The owner's 0.3.7 device returns measured OFF in valid Hue JSON even while the
app displays ON after a fresh GET. Related community observations appear in
[Espalexa issue 90](https://github.com/Aircoookie/Espalexa/issues/90) (external
state changes and app display) and [Home Assistant issue 30013](https://github.com/home-assistant/core/issues/30013)
(Hue on/off profile compatibility, mostly discovery reports). These are historical
reports, not proof of the current cause or of a successful fix. The additional
binary bri/mode fields in 0.3.8 are an experiment; the device remains an On/off
light and does not accept dimming commands. Actual Alexa behavior requires testing.

The optional 0.3.9 profile uses the pinned Espalexa dimmable-device metadata
(`Dimmable light`, `LWB010`) as a controlled alternative to its on/off profile.
Historical Home Assistant reports describe successful dimmable-profile workarounds
for some Alexa devices, but are not validation for the owner's current Echo/app.
The bridge's brightness handling remains explicitly binary ON/SLEEP.
