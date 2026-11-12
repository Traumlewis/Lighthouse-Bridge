# Experimental Matter prototype — 0.2.0-matter

This next test exposes **up to 16 configured Lighthouse V2 stations as separate
On/Off Lights** on the original ESP-WROOM-32 / ESP-32S with at least 4 MB flash.
The owner confirms that the prior single-station 0.1.1 test switches ON/OFF and
shows the correct light type/status. Multiple stations, long-run reliability,
specifically external state changes and the eventual full firmware still need
hardware testing. No ESP32 or Echo is attached locally.

The test bridge name is **Lighthouse-Bridge Matter Test**. The owner wants the
finished bridge named **Lighthouse-Bridge** after testing. Station names are read
from the saved configuration and are not translated or renamed.

## Install and test

1. Keep Wi-Fi and up to 16 station assignments saved with stable firmware 0.3.9.
   If these are already saved from your previous test, update directly. This
   prototype reads the same record; it does not provide a station editor.
2. Open [the Matter test WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/matter.html),
   connect the classic ESP32 by USB, and install. **Leave Erase device unchecked.**
3. Close the installer. The USB panel reads the setup code and every configured
   station's measured state, with its actual fixed local ID. Alternatively click
   **Read setup code & status**. The ESP32 joins its saved Wi-Fi.
4. Existing Matter credentials and the first prototype station's identity are
   retained. Check Alexa for the additional lights and the updated bridge name.
   If Alexa caches the old list/name, remove only **this experimental Matter
   bridge while the ESP32 is connected**, then pair it again with the displayed
   USB code. Removing the bridge can remove all its children and their group/
   routine assignments; restore these if necessary. Do not erase the ESP32.
5. For a new pairing, add a Matter device in Alexa and enter the code manually.
   Your Echo must support Matter and share the home network. Test attestation is
   not commercial certification; Alexa may ask to accept an uncertified device.
6. Test every station individually: ON, then SLEEP, and its Alexa status. Check
   that no other station switches. If desired, put the lamps into an Alexa group
   and test collective ON/SLEEP. The bridge does not create a virtual group light.
   Allow for sequential BLE work and booting/confirmation; a sent command is not
   the same as an observed steady ON (raw 11) or SLEEP (raw 0).
7. Report the station count and any switching/status issue. Useful serial lines
   start with `Matter command`, `Matter measurement`, or `Station`. Do not share
   setup codes, API keys or credential-bearing logs.

The next phase restores the dashboard, local HTTP API and Wi-Fi firmware upload
after this multi-station test and additional size optimization. **They are not
included yet.** The early full-services build probe still exceeds the fixed app
slot, even with a gzip UI. Return to stable using the stable USB WebFlasher with
erasing unchecked. Existing Hue entries are separate and unavailable during the
Matter test. Changing assignments requires stable firmware for now.

## Storage and identities

Both **1.875 MiB app slots**, NVS `0x9000..0xdfff`, OTA metadata and the unused
filesystem region retain the exact stable partition table. USB writes only four
separate files at `0x1000`, `0x8000`, `0xe000`, `0x10000`. No merged-at-zero image
or `.lhota` is offered for this prototype. The application must fit `0x1e0000`.

The shared station parser loads the stable record without rewriting it. The
separate `lhmatter` namespace keeps per-board commissioning credentials and an
atomic `endpoints` registry binding full stable Hue identity + MAC to a Matter
endpoint. The original first station retains endpoint 1; aggregator 2 is reserved.
Other endpoints start at 3 and increase monotonically. Reordering never renumbers
survivors; a replacement MAC or new identity gets a new endpoint. Retired endpoints
are not reused. Corrupt/exhausted registries fail closed, without erasing settings.
Current+candidate checkpoints can hold 32 bindings while active capacity remains
16. Stable local IDs remain 1–16 and are distinct from Matter endpoint IDs.

The original single-station migration assumes its saved assignment was retained.
If you replaced assignments while running stable firmware between tests, refresh
the experimental Alexa pairing rather than assuming its cached device is unchanged.
Matter also persists fabric data; long-run NVS behavior and returning between
framework versions remain hardware checks.

## USB and builds

`LH_MATTER_INFO\n` at 115200 baud returns a `LH_MATTER_INFO` JSON frame with protocol
1, version, bridge name, readiness/error, Wi-Fi, pairing state/code and a measured
`stations` list. Legacy first-station fields remain for older readers. It exposes
no station MAC, stable API key or Wi-Fi password. Codes stay in browser memory,
never in URLs, storage or network calls. Disconnect USB reading before flashing.
Language changes retain the USB connection and experimental consent checkbox.

Install Arduino CLI **1.6.0**, then:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install 'NimBLE-Arduino@2.5.2' 'ArduinoJson@6.21.5'
python3 tools/build_matter.py
```

The stable PlatformIO environment remains Arduino ESP32 2.0.17. The isolated build
uses 3.3.12 with CHIPoBLE commissioning disabled, leaving NimBLE 2.5.2 to the one
sequential Lighthouse worker. `tools/build_matter.py` generates that worker header
from the production section of `src/main.cpp`; there is no second retry loop.
The root request/measurement logic remains shared. Boot and reconnect never send
automatic power commands. Matter command interception queues work; only observed
OnOff/Reachable updates are reported on the Matter task.

The build validates classic ESP32 images, both slots, all partition entries,
sector bounds and per-image hashes. `dist/matter/` contains split images, manifest,
checksums, source/notes/licenses ZIP and build metadata. Build both packages before
`tools/build_pages.py`; the existing Pages workflow publishes separate stable and
experimental choices. Software tests do not establish successful multi-station
operation, runtime RAM margins, or Alexa discovery/cache behavior.
