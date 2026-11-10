# Validation report — prototype 0.2.1

## Intermittent BLE failures: 0.2.1

The owner reports intermittent connection failures and previous measurements
being shown as unreachable, including without another controller accessing the
stations. Version 0.2.0 allowed one read attempt with a three-second timeout and
marked the first failure as unreachable. This does not establish a specific radio
or hardware cause.

Version 0.2.1 uses a six-second timeout and two attempts per read cycle, separated
by a short pause. After one failed cycle it retries after five seconds. A usable
previous measurement remains explicitly labeled **last reading** without changing
its timestamp. Two failed cycles, no previous measurement, or age of at least
60 seconds still means unreachable. Numeric BLE errors are exposed for diagnosis.
Commands retain priority.

Native regression tests cover single/repeated failures, recovery, age expiry,
counter saturation, missing initial data, millis wraparound and discarded readings
after reassignment. Browser tests cover English/German last-reading labels and
the transition to unreachable. A separate BLE-active flag prevents Wi-Fi setup
from starting while a reassigned device's previous read is still in flight.
The effect on actual connection reliability still needs testing on the board.

## Owner feedback on 0.1.1 and the 0.2.0 additions

The owner confirmed successful startup and basic operation of 0.1.1, but reported
lost Wi-Fi settings after updates and inconsistent dashboard/Alexa state.
The update-loss cause is demonstrated by the image format: merged firmware
contains padding over NVS. Deselecting a full erase was not sufficient.

Version 0.2.0 uses four separate images. A regression test simulates sector
erases/writes over existing NVS content and confirms `0x9000..0xdfff` remains
unchanged. The factory image reproduces the previous overwrite. Split images
match the corresponding bytes of the patched merged image.

Firmware reads at startup, periodically and on explicit refresh without a power
write. Native tests check measured versus requested state, failure/staleness and
millis wraparound. Hue GETs use the same readings but cannot push state changes
into the Alexa app.

Browser tests with mocked HTTP/USB passed for:

- Personal-password setup after API-key bootstrap, mandatory password setup,
  rejecting the key for later web login, and showing the key in the signed-in
  network settings section.
- Logout/relogin, English/German switching and mobile layouts.
- State displayed before the first power command, no fabricated optimistic
  measurements, updated readings and stale readings.
- USB stream parsing, fragmented frames, invalid URL rejection, Wi-Fi address
  changes, disconnect and port handoff after the installer dialog closes.
  Keys are not persisted or transmitted to the hosting service.

Builds, native tests, Python layout/integrity checks and Node USB tests passed.
**The expanded functionality still needs end-to-end board/Echo validation.**
Browser mocks do not verify on-device password hashing/session endpoints, Wi-Fi
setup while querying USB, NVS retention after a real flash, continuous BLE reads,
or Alexa's actual polling/cache behavior.

## Initial hardware report and startup correction

On November 10, 2026, the owner reported the first real 0.1.0 boot: firmware
started, printed an access key and joined home Wi-Fi, then aborted during Bluetooth
initialization. Local symbols from the corresponding source map the trace to:

`coex_core_enable → coex_enable → esp_bt_controller_enable → NimBLEDevice::init → setup`

The exact ELF from that original CI build was not retained. Subsequent builds
retain matching symbols in the `lighthouse-bridge-debug` Actions artifact.
Personal device/network identifiers and access keys from the report are not
stored in the repository.

Cause: `WiFi.setSleep(false)` selected `WIFI_PS_NONE` before BLE startup.
Version 0.1.1 explicitly selects `WIFI_PS_MIN_MODEM` and checks the driver result
before initializing BLE. Arduino's cached policy is set before STA startup so
later restarts cannot reapply `WIFI_PS_NONE`. Readiness and web-address messages
appear only after initialization. This Wi-Fi setting does not change Lighthouse
SLEEP commands. The owner subsequently confirmed successful 0.1.1 startup.

Reference: [Espressif IDFGH-8094 / issue 9595](https://github.com/espressif/esp-idf/issues/9595).
The missing core-dump-partition message is a separate diagnostic limitation:
the existing partition table has no core-dump partition. The startup fix did not
change partitions.

## Development-environment checks

- PlatformIO `espressif32@6.9.0`, `esp32dev`, Arduino ESP32 2.0.17.
- Firmware fits the 1.875 MiB application slot. The first successful build used
  about 1.20 MB flash and 59 KB static RAM; runtime heap/task-stack consumption
  must be checked on the device.
- Native C++ tests: ON/SLEEP-only commands; rejection of STANDBY/invalid input;
  MAC/name validation; starting-state decoding; superseded command/read results;
  no invented measurements after failures; transient health and age limits.
- Playwright/Chromium tests exercise the embedded HTML with mocked API responses:
  four initially unassigned slots, discovery/assignment including random address
  type, preservation of edits during polling, save errors, power actions,
  measurement failures and authentication/language flows.
- Layouts at 1280 and 390 pixels without horizontal overflow. Screenshots are
  simulated examples, not measurements from real stations.
- Factory image: bootloader, partition table, boot metadata and application bytes
  match their target offsets. ZIP integrity and SHA-256 are checked at packaging.
- Current WebFlasher: classic ESP32-only four-part manifest, published checksums,
  relative project paths, complete lazy modules and NVS-preserving sector ranges.
  The older merged-at-zero manifest was used only before 0.2.0.
- Browser checks load the real vendored installer modules and invoke a mocked
  USB picker. No real USB device is accessed. Missing firmware disables install
  and download. English default, German switching, saved preference, URL override,
  translated messages/metadata and preserved confirmation are checked.

## Remaining hardware checks

1. Confirm exact board/flash size, USB upload, key persistence and an actual
   settings-preserving upgrade using the separate parts.
2. Test correct/incorrect Wi-Fi credentials, portal timeout, BOOT recovery,
   reconnect and USB information while the portal is active.
3. Test web/session/API credentials on the device, personal-password storage and
   USB password recovery.
4. Discover and assign all four Lighthouse V2 stations. Check each station with
   ON and **SLEEP**, never STANDBY.
5. Verify individual/group actions, BLE range, retry behavior and unreachable or
   unpowered stations; retest the intermittent failures reported by the owner.
6. Test rapid ON → SLEEP requests: the newest target must execute last. Reboot
   must preserve configuration without sending a power command.
7. Check Echo Dot 5 discovery, five Hue lights, voice commands, renaming and
   measured-state queries. An acknowledgement is not confirmation of BLE success,
   and app caching remains controlled by Alexa.
8. Check longer-running heap usage, HTTP responsiveness during BLE work,
   concurrent SteamVR use and recovery after power/network interruptions.

Builds and browser simulations do not replace hardware tests. No board is
attached to this development environment. Owner feedback confirms startup/basic
operation and intermittent BLE read failures, not complete validation of every
feature and edge case.
