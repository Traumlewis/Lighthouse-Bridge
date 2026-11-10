# Validation report — prototype 0.2.4

## BLE host upgrade and clearer read phases: 0.2.4

The owner confirms all four stations were already configured with Random (1).
The temporary Public (0) test produced NimBLE host timeout code 13; type 0 is
not a remedy. The subsequent 0.2.3 log still finds connectable ADV_IND/type-1
advertisements, with RSSI ranging from -55 to -85 dBm, alongside repeated 574
failures and successful SLEEP reads. The UI reports transient read failure,
unavailable measurements and reading in progress. A saved-type mismatch is no
longer the working explanation. No specific RF or board cause is established.

Version 0.2.4 migrates from NimBLE 1.4.3 to pinned 2.5.2. Upstream 2.5.0 added
internal connection-establishment retries for HCI 0x3e; 2.5.2 includes subsequent
fixes. The bridge explicitly sets one host retry, rather than relying on the
library default of two. A controller attempt uses 3000 ms; two worker read
attempts allow at most four controller starts, and three command/write attempts
allow at most six. Recovery scans and worker priority checks remain. This does
not increase the maximum number of power writes beyond three per command job.

The API migration uses blocking `getResults(2000/5000, false)` for the original
2/5 second scans, copies const result pointers before clearing, and uses the
2.x millisecond connection timeout. BLE initialization is checked before claiming
readiness. Since 2.x client deletion can be deferred, the worker waits for client
cleanup before another scan/connection and keeps the physical BLE-active flag
set meanwhile. It never dereferences a client after deletion is requested. A
cleanup wait over five seconds logs a warning while HTTP continues; no overlapping
radio work is forced and cleanup does not refresh measurement timestamps. Every successful read is logged, not only recovered reads; failed
worker attempts include address type and host retry limit. Protocol, partition
layout, Wi-Fi coexistence policy, saved settings and authentication are unchanged.

Status preserves aggregate `reading` and adds `read_active` / `read_queued`.
The dashboard distinguishes waiting from active work and calls the displayed age
**last successful reading**. Recent measurements invalidated by failures say
unavailable; only age of at least 60 seconds says stale. This does not loosen
reachability or conceal connection errors. Native state tests and English/German
browser cases cover failure/expiry and queued/active labels respectively.

The final firmware build and native state tests passed. Seven Python
translation/layout/NVS tests and the browser/USB suites passed locally. Browser
regressions cover queued versus active reads in English/German, recent unavailable
versus expired measurements, and explicit last-measurement age labels. These are
mocked HTTP/USB checks, not measurements of BLE recovery on a board.

**Hardware effectiveness is unconfirmed.** The upgrade exercises an upstream
recovery path for this error; it does not establish the cause or guarantee a fix.

## Owner log from 0.2.3: advertising visible, connections still intermittent

The supplied log covers approximately 13:53–13:56. All 23 recovery scans found
connectable ADV_IND (legacy report type 0) from the assigned station, always with
random address type 1. Reported RSSI spans -60 to -76 dBm. There are 29 failed
connection attempts and 12 recovered reads. The log records ON and SLEEP writes
reported sent for all four stations, with measured raw 0 (SLEEP), 1 (starting)
and 11 (ON) appearing in recovered reads. A sent write alone is not a confirmed
measured state. First-attempt successes are not logged, so this excerpt cannot
be used to calculate an overall failure rate.

This confirms that the corrected filter sees the stations; the earlier negative
scan diagnosis was misleading. It does not establish an RF, timing or board
cause. A specific configuration check is now pending: compare the saved address
types with the observed type 1. The worker copies the scan's type only into the
current job; a wrong saved type would be reused by the next first attempt.
The owner subsequently confirmed all four were already set to type 1; the
saved-type mismatch hypothesis was not supported.

## Legacy recovery scan correction: 0.2.3

The owner's 0.2.2 log confirms intermittent successful reads with `observed=0`
(SLEEP) on stations 1 and 2, alongside repeated 574 connection failures across
all four stations. The owner reports about 1.60 m distance and failures both
with green (ON) and blue (SLEEP) LEDs; the problem is not confined to SLEEP.
Negative recovery scan messages precede both successful and
failed retries. Recovery is therefore incomplete; no radio cause is established.

Inspection of the pinned NimBLE 1.4.3 source reveals a deterministic bug in its
classic ESP32 `isConnectable()` path: legacy report type enums are interpreted
as extended-advertising bit flags. Ordinary connectable ADV_IND (0) is rejected;
non-connectable ADV_NONCONN_IND (3) and SCAN_RSP (4) are accepted. The bridge's
0.2.2 retry filter called that method. A host reproduction using the actual
function extracted from the installed dependency confirms all three errors.
Thus the previous negative scan message was not reliable evidence of absence.

Version 0.2.3 uses explicit legacy report classification: only ADV_IND (0) and
ADV_DIRECT_IND (1) are connectable. Firmware static assertions verify these
values against the pinned HCI definitions. Native regression tests cover report
types 0 through 4 and an invalid type. A scan now distinguishes no matching
address from a matching non-connectable report, with report type, RSSI and total
scan result count. Address matching, bounded retries and read-only recovery
remain unchanged. The dependency and connection parameters are unchanged.

The firmware build, native tests (including the new legacy report cases), seven
Python layout/NVS/translation checks and USB/layout tests passed locally. The
upstream bug was reproduced on the host from the installed source; no board is
attached to the development environment.

**This fixes the scan filter and diagnostics, not a proven cause of error 574.**
The next hardware test must collect accurate advertisement visibility and RSSI;
intermittent successful reads alone do not demonstrate reliable operation.

Reference: [pinned upstream implementation](https://github.com/h2zero/NimBLE-Arduino/blob/1.4.3/src/NimBLEAdvertisedDevice.cpp).

## Connection establishment error 574: 0.2.2

The owner reports repeated numeric code 574 across all four stations, also without
another controller. NimBLE 1.4.3 defines HCI errors as 0x200 + controller code:
574 = 0x023e = HCI 0x3e, `BLE_ERR_CONN_ESTABLISHMENT`. The previous generic
`READ_FAILED` serial line obscured the operation's actual failing phase. The code
alone does not prove interference, weak signal, incorrect addressing or a faulty
board. Increasing a read timeout alone does not resolve this controller error.

Version 0.2.2 keeps the pinned BLE dependency and bounded attempt counts. Before
retrying this code it performs a two-second passive scan. It matches only the
assigned MAC, requires connectable advertising and copies the observed address
type into the current job, never into persistent settings. If the scan misses it,
the saved address is still retried. Scan results are cleared without replacing
the user's discovery list. Retry delays vary; read retries check pending commands
and reassignment both before and after scanning. Serial output records each
failed operation/attempt/code, and recovery advertising RSSI/type or its absence.
The observed-state grace, expiry and ON/SLEEP-only rules are unchanged.

The 0.2.2 firmware build, native core tests, seven Python translation/layout/NVS
checks and browser/USB suites passed locally. Browser tests use mocked HTTP/USB;
they do not exercise the new radio recovery on hardware.

This is a recovery and diagnostic change, **not a confirmed hardware fix**. The
next board test should compare failures and recoveries with RSSI and advertising
visibility. The environment has no attached ESP32 or base stations.

References: [NimBLE return codes](https://github.com/apache/mynewt-nimble/blob/master/docs/ble_hs/ble_hs_return_codes.rst),
[maintainer advice to retry after rediscovery](https://github.com/h2zero/NimBLE-Arduino/issues/140#issuecomment-769411985).

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
