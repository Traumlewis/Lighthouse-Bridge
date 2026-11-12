# Experimental Matter prototype — 0.1.1-matter

This is a separate, single-station experiment for the original **ESP-WROOM-32 /
ESP-32S with at least 4 MB flash**. It is not a replacement for stable 0.3.9.
The owner reports that the first prototype works fundamentally on hardware,
but Alexa correctly classified that version's advertised endpoint as an outlet.
This update advertises an **On/Off Light** and needs a hardware retest. Detailed
confirmation timing, runtime memory, radio reliability and app state updates
remain unverified. Build/packaging/browser checks are separate software evidence.
No Echo or ESP32 is attached to the development environment.

## Try it using the existing WebFlasher

1. Use the stable firmware to save Wi-Fi and at least one Lighthouse V2 assignment.
   Note which configured station has the lowest local ID: that is the only station
   this prototype uses. Keep a copy of your configuration before testing.
2. Open [the experimental WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/matter.html).
   Connect the classic ESP32 with a USB data cable and install the prototype.
   **Leave “Erase device” unchecked.** The prototype needs the saved configuration.
3. Close the installer. The page reads the prototype's information over USB;
   alternatively click **Read setup code & status**. The ESP32 joins its saved Wi-Fi.
4. In Alexa, add a Matter device and enter the displayed setup code manually.
   A compatible Matter-capable Echo must be on the same home network. The prototype
   uses Espressif's test attestation identity, not a certified commercial identity;
   Alexa may require accepting an uncertified test device or may reject pairing.
5. Test ON and SLEEP for the single bridged on/off light endpoint. The bridge reports
   actual BLE measurements and measured reachability. Acceptance queues a command;
   ON is confirmed only by raw 11 and SLEEP only by raw 0. Booting remains active.
   Serial command history distinguishes sent, confirmed and unconfirmed work.
6. To return, use the stable USB WebFlasher with erasing unchecked. Remove the
   experimental Matter device from Alexa when finished. Existing Hue entries are
   separate from the new Matter endpoint and will be unavailable during the test.

### Updating the outlet prototype

Version 0.1.0-matter advertised an On/Off Plug-in Unit. Version 0.1.1-matter
changes that descriptor to On/Off Light while keeping the endpoint ID, unique ID,
commissioning credentials and partition table. Update over USB with erasing
unchecked. Alexa may retain the outlet type of an existing entry; if so, remove
only this experimental Matter device while the ESP32 is connected and pair it
again with the USB-displayed code. It is also possible to remove the old entry
before updating. Groups/routines referencing it may need restoring. Do not erase
the ESP32 or reset stable Wi-Fi/station settings to refresh the Alexa type.

The prototype has no dashboard, local HTTP API, station editor, discovery UI or
Wi-Fi firmware upload. Configure assignments before installing it. If you change
the selected station in stable firmware, remove/re-pair the experimental Alexa
device rather than assuming a cached Matter endpoint still refers to the old one.
There is no automatic power command on boot, reboot or Wi-Fi reconnect.

## Storage and USB

Both 1.875 MiB app slots, NVS `0x9000..0xdfff`, OTA metadata and the unused filesystem
region retain the **exact stable partition table**. USB writes four separate files
at `0x1000`, `0x8000`, `0xe000`, `0x10000`; no merged-at-zero image is offered.
The application must fit `0x1e0000` bytes. The stable firmware's configuration is
read through the shared migration/validation parser, without rewriting its record.
Matter fabric storage and per-board commissioning credentials also use NVS, with
the latter in a separate `lhmatter` namespace. Runtime persistence and returning
between framework versions still require hardware testing.

`LH_MATTER_INFO\n` at 115200 baud returns `LH_MATTER_INFO {protocol:1,...}` with
selected local ID/name, readiness/error, Wi-Fi, pairing state, unpaired setup code
and measured station state/history. It does not expose the stable API key, Wi-Fi
password or station MAC. The WebFlasher keeps the code in page memory and sends it
only across physical USB; it never places codes in URLs, storage or network calls.
Only one serial owner is allowed; disconnect the reader before flashing again.

## Build

Install Arduino CLI **1.6.0**, then:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install 'NimBLE-Arduino@2.5.2' 'ArduinoJson@6.21.5'
python3 tools/build_matter.py
```

The stable PlatformIO build remains Arduino ESP32 2.0.17. The isolated prototype
uses Arduino ESP32 3.3.12 and on-network commissioning, reserving NimBLE 2.5.2 for
the sequential Lighthouse worker. Matter's CHIPoBLE commissioning is compile-time
disabled for this classic-ESP32 build. Do not replace NimBLE with Arduino's BLE
wrapper: a minimal Matter+Bluedroid probe already exceeded the existing app slot.

The build refuses a different installed core, oversized segments, non-classic
images, any changed partition entries or writes touching NVS. `dist/matter/` holds
the split images, per-image SHA-256 manifest/checksums and prototype ZIP. Generate
both firmware packages before `python3 tools/build_pages.py`. CI publishes both
choices through the existing Pages deployment; the stable manifest stays separate.

The prototype uses the project's shared ON/SLEEP, command coalescing, generation,
read-failure and confirmation rules. Its command handler accepts asynchronous
work without changing the measured Matter OnOff attribute. Actual observations
are reported on the Matter task; reachability uses the bridged-device cluster.
The exact behavior with Alexa and the SDK command-handler interception must be
verified on hardware before treating this as a working replacement.
