# Validation report — 0.3.7

## Version 0.3.7 fixed reusable station slots

The owner clarified that stations receive fixed IDs from a pool of 16 slots,
not IDs that shift on removal. The lowest free ID is assigned on creation;
remaining IDs survive removal/reordering and reboot. Retained slots are reserved
before allocation, so a new entry earlier in a payload cannot steal an existing
ID. Records/responses are sorted by ID, with gaps allowed. UI headers/default
names preview the same slot selection. Stable Hue identities remain separate.

Native checks cover deletion without renumbering, freed-slot reuse, reload with
gaps, allocation before retained entries, all 16 slots, and previous migrations,
identity/history preservation and generation checks. Browser checks cover retained
card IDs after deletion, default name/header matching the freed ID, reuse, and
commands sent to the fixed ID. Existing debug geometry/auth/language checks remain.
Firmware/package/NVS checks apply; simulated tests do not use physical hardware.

## Version 0.3.6 contiguous local station IDs

The owner requested actual IDs 1..N as well as card labels 01..N, rather than
exposing old monotonic IDs such as 6–9. Configuration now separates contiguous
local IDs from stable Hue identities. Old bare-slot/dynamic records migrate
without changing Alexa keys or unique IDs. New NVS records persist both ID spaces
atomically; clients cannot overwrite Hue identities. Local ID 5 is valid while
retired Hue identity 5 stays unavailable.

Native tests cover the owner's 6–9 to 1–4 migration, serialization/reload,
removal/reorder/addition, all 16 local IDs, Hue-to-local command mapping including
local station 5, preservation of measurement/history by Hue identity+MAC, and
invalidation of stale reads. Browser checks cover labels 01..16, compaction after
removal, local-ID control, controls blocked until the new mapping is loaded, and
retained Alexa identities in the mocked backend.
Firmware/package/NVS checks apply; no board is attached. Actual Alexa continuity
through this migration remains an owner hardware check.

## Version 0.3.5 quiet view with stable layout

The owner reverted to the quiet 0.3.3-style normal view, asking that Debug not
shift the design. Card status areas now have a fixed four-line height in both
modes; long diagnostics scroll internally. Badge height stays fixed, and the
footer reserves indicator space even while its label is invisible. Debug can be
scrolled using keyboard focus. Measured state/unavailability and queued/sending
commands remain visible normally; detailed read phases, errors, history and ages
require Debug. No underlying BLE/API/Hue behavior changes.

Browser checks compare document-space card, status-area, action and footer
positions/sizes before and after toggling on desktop and EN/DE mobile layouts.
They verify that long diagnostics overflow internally and that normal-mode
filtering/toggle timing/language/reset behavior remains correct. Firmware and
package/NVS checks apply. No physical device is used for these browser checks.

## Version 0.3.4 warning-only debug filtering

The owner clarified that only the orange warnings should be hidden, rather than
all status details. The dashboard now hides BLE error/retry text and orange
warning styling while debug is off. Regular background-read/queued phases,
last-command results, measured-state badges, last-reading suffix, ages and
unavailable/stale text stay visible. The triple-click toggle/reset behavior and
all underlying API/BLE/Hue behavior are unchanged.

Browser checks cover normal mode retaining ages, command-confirmation history
and background-read phases while suppressing read errors and orange styling;
debug mode restores warnings. Unavailable stations remain explicitly labeled in
normal mode. Existing toggle timing, keyboard, language and reset checks remain.
Build/package/NVS checks apply; the browser backend and radio are simulated.

## Version 0.3.3 dashboard debug presentation

The owner requested hiding verbose per-station errors, measurement ages and
last-command history behind a three-click version toggle. The mode is off by
default and on logout/reload, is not stored, and preserves its setting across
language changes. Three activations within 1.2 s enable/disable it. Native button
keyboard activation is supported. Current measured badges, unavailable status,
first-read wait and queued/sending commands remain visible in normal mode.

Browser checks with mocked station states cover normal/debug rendering, single
and double clicks, expired click sequences, repeated toggling, EN/DE, confirmed
command history, retained measurements during one failed read, unavailable/stale
readings and logout reset. Toggling must not queue commands or refresh writes.
API/serial fields, BLE retries, Hue views and flash layout remain unchanged.
The firmware build, package/header checks and NVS-preservation tests still apply;
no physical flash is performed by the browser tests.

## Version 0.3.2 Hue discovery identifier regression

The owner reports Alexa cannot find the four stations on 0.3.1. The excerpt shows
newly assigned persistent IDs 6–9, successful BLE readbacks, and repeated Hue
collection GETs with configured=4/fresh=4 initially. Alexa can reach the bridge;
that log does not contain the actual HTTP JSON or prove why Alexa rejects devices.
Intermittent BLE 574 failures also occur, but do not establish a discovery cause.
The raw excerpt contains credentials and is not committed.

Code inspection reproduces a separate concrete discovery regression introduced in
0.3.0: `uniqueid` for IDs >4 has an unpadded decimal endpoint (`-6-`) instead of
the pinned Espalexa hexadecimal endpoint (`-06-`). Version 0.3.2 restores the
previous formatting for IDs 1–255. Larger monotonic IDs have a fixed EUI-64 form
instead of an overlong endpoint. Numeric keys, saved assignments and first-four
identities are preserved. No BLE timing or power behavior changes.

Native tests now use the production discovery serializer, not only the write-route
parser: 16 entries, the owner's IDs 6–9, hexadecimal IDs 10/17/255, unique fixed
formats for IDs beyond 255, no retired group, JSON roundtrip after temporary
strings leave scope, and measured/accepted-target separation in state-only GETs.
The firmware build and package checks exercise the corrected code. No Alexa or
ESP32 is attached; the owner must retest actual device discovery after updating.
This is a verified code/format regression fix, not a guarantee that every Alexa
firmware will accept every identifier variant or that all discovery causes are fixed.

## Version 0.3.1 embedded key styling

The dashboard now matches the public WebFlasher's plain clickable API-key text,
with no visible button background, border or padding. Browser checks cover those
computed styles, >=4.5:1 contrast, local-HTTP click and keyboard copy fallback,
and mobile width with the key visible. Firmware, embedded footer, USB/Hue version
and update packages are 0.3.1. No BLE, authentication or flash-layout behavior changes.
The styling checks use a simulated authenticated device; no physical ESP32 is attached.

## WebFlasher key contrast correction

A browser regression reproduces the 0.3.0 copy-button defect: the inherited
accent background and USB code text are identical, giving a 1:1 contrast ratio.
The public page now shows plain clickable accent text, with no button background,
border or padding, as requested by the owner. Feedback appears on a separate line. The regression checks >=4.5:1 contrast
with the key visible, mobile width, the fallback copy path and Chromium's actual
Clipboard API through keyboard activation. USB is simulated; the clipboard is
real within the test browser. Firmware remains 0.3.0; no ESP32 update is needed.

## Version 0.3.0 software validation

No ESP32 is attached to this environment. Earlier hardware evidence below concerns
four physical stations and previous versions. It does not establish OTA behavior
or operation with 16 physical stations.

- Classic ESP32 PlatformIO build succeeds within the unchanged 1.875 MiB app slot
  (about 1.26 MB flash / 63 KB static RAM). Runtime heap remains a hardware check.
- Native production-header tests cover legacy four-slot migration, removal of empty
  legacy slots, 16-entry capacity, stable monotonic IDs, retired group ID 5,
  reordering/history preservation, generation invalidation of in-flight reads,
  duplicate/malformed records, mandatory Random (1) and device-name validation.
- Hue tests cover 16 individual devices, exact per-ID boolean acknowledgements,
  unavailable retired group, duplicate request coalescing, malformed requests and
  measurement separation. Existing request/health/confirmation tests still pass.
- OTA header/container checks cover capacity/chip/product, reserved bytes,
  digest/length, truncation, trailing data, wrong chip, merged BIN and corruption.
  Packager cross-checks the .lhota payload against the USB application image.
- Native tests also drive the production OTA receiver with arbitrary 1–2048-byte
  chunks and fake inactive-slot IO: prefix-before-write, length/product/digest
  rejection, write failures, explicit abort after file-end and commit separation.
  Hash IO is deterministic in that test; real SHA-256 is checked in Python.
- Browser checks use the actual embedded/public HTML/JS with mocked HTTP/USB.
  They exercise password bootstrap, EN/DE, editable drafts surviving polling,
  address suggestions without a type selector, 16 dynamic entries, removal and
  non-reused IDs, empty list, background reads with an enabled Wi-Fi button,
  key copying via the local-HTTP fallback, custom hostname, multipart token upload,
  waiting, rejected upload/cancellation and successful restart messaging.
  Firmware writing and reboot in those browser tests are simulated.
- Python/Node checks verify local assets, translations, USB parser/handoff,
  SHA-256 URLs and sector-level NVS exclusion of all four Web Serial parts.
  Vendor modules/licenses and partition offsets are unchanged.
- Firmware upload auth/abort paths were reviewed against the pinned Arduino
  WebServer and Update libraries. Update.end (boot-slot selection) occurs only
  after complete authenticated multipart acceptance, size/product/SHA validation.
  Invalid/aborted writes abort the inactive-slot writer. These are software checks,
  not evidence from a physical interrupted flash or power-loss experiment.
  WiFiManager raw-BIN routes are shadowed before vendor registration; only the
  authenticated main interface is intended for firmware upload.

Required owner retest: upgrade once by USB with Erase unchecked, verify saved
settings and existing individual Alexa IDs, delete the old cached virtual group,
then test .lhota success, wrong-file rejection and interrupted upload on the ESP32.
Check custom SSID/mDNS and portal appearance on a phone, actual Alexa discovery,
and longer-running heap/radio behavior. Sixteen-device polling may take longer
than the target interval; fresh/reachable status remains tied to real timestamps.

## Additional 0.2.7 SLEEP monitoring sample

The owner supplies a continuation from approximately 15:59:23 to 16:07:21
(just under eight minutes). It contains no power commands and all successful
readbacks are raw 0 (SLEEP). This is an idle-monitoring/recovery sample, not
another switching or command-confirmation test.

| Slot | Successful read reports | First worker attempt | Second worker attempt | Largest gap between successful readings |
| --- | --- | --- | --- | --- |
| 1 | 28 | 22 | 6 | 20 s |
| 2 | 28 | 25 | 3 | 27 s |
| 3 | 28 | 23 | 5 | 23 s |
| 4 | 28 | 26 | 2 | 22 s |

There are 112 successful read reports and 18 failed worker-attempt reports.
Sixteen reads recover on their second worker attempt. One station-2 cycle
exhausts both attempts at about 16:06:56, then a later cycle succeeds at
16:07:05, nine seconds later. No two consecutive exhausted cycles are shown.
Counts do not expose internal host retries and must not be presented as the
physical link's first-attempt success rate.

All eight logged Hue GETs report on=false, reachable=true, pending=false and
source=measurement. Recorded gaps between successful readings remain below
the 60-second expiry threshold. The evidence supports recovery from a failed
read cycle without inventing state or prematurely expiring the previous reading.
It does not establish the source of 574, continuous operation over hours/days,
or behavior during another controller's simultaneous access. Firmware and radio
settings remain unchanged; only this aggregate report and project instructions
are updated.

## First 0.2.7 hardware confirmation run

The owner reports further improvement and supplies approximately 3.5 minutes of
0.2.7 station/Hue output. All eight shown power jobs reach their requested state:

| Target | Station results | Observed confirmation |
| --- | --- | --- |
| ON | All four stations | Immediate reads show raw 8, follow-up reads raw 9, then raw 11 with `command target confirmed`. Confirmation follows the sent reports by about 15–16 seconds. |
| SLEEP | All four stations | Each write report includes raw 0 and `confirmed=true`. |

The excerpt contains 51 successful read reports and five failed worker-attempt
reports with code 574. Station 2's initial exhausted read cycle recovers on later
polling; other retry reads recover, and the last station-3 read failure is followed
by a successful SLEEP job. **No shown power job ends in failure.** Worker report
counts do not expose internal host retries or constitute a controlled estimate
of physical-link reliability.

Hue replies use `source=accepted_target` immediately after ON requests and
`source=measurement` afterward. Later group queries show measured ON/SLEEP with
reachable=true. This run confirms the new sent-versus-confirmed behavior on the
owner's board and all four stations. It does not establish continuous reliability,
all failure paths, Wi-Fi/NVS update retention or every Echo behavior. Raw boot
output with credentials/device identifiers is not committed. Only this summary
is retained; no firmware or retry changes are justified by this run alone.

## Follow-up after Alexa improvement: 0.2.7

The owner reports improved Alexa behavior with 0.2.6. The follow-up log shows
accepted-target Hue replies during work and measured replies afterward. The
shown ON operations progress through raw 9 to steady 11, and SLEEP operations
are followed by raw 0 on all four stations. Intermittent 574 connection/read
failures remain, including exhausted read cycles that recover on later polling.
No physical cause or complete BLE reliability is established by that excerpt.

Code inspection found that a successful Write Without Response could be called
sent even if its immediate read was missing or still showed a different state.
Version 0.2.7 makes confirmation explicit. Successful writes confirm only on
raw 11 for ON or raw 0 for SLEEP. Otherwise the worker uses existing pure reads
about every two seconds for up to 30 seconds after sending, yielding to queued
commands. No retry limits or transport parameters change, and no extra power
writes occur. A failed write does not open verification. Deadline expiry marks
history unconfirmed; later polling still updates measured physical state.

Confirmation latches for the last command, independently from later external
changes. New commands reset it; mismatched-generation readings cannot confirm
them. The existing Hue target view still ends on command-worker completion;
verification does not extend optimistic reporting. API/UI expose the last-command
phase separately from current measured badges. Status time is captured under
the state lock to keep measurement ages coherent during concurrent worker updates.

Native tests cover immediate/later confirmation, steady ON versus booting,
SLEEP versus STANDBY, missing reads, failures, deadline boundaries, late matches,
external changes, superseded results and millis wrap. Browser cases cover
English/German verification/history labels without inventing an ON measurement.
The final firmware build and native core/Hue tests passed, followed by seven
Python translation/layout/NVS checks and the browser/USB suites. Browser cases
verify that last-command history can differ from the current measured badge
without changing it. These are host/mocked checks, not an on-board confirmation
of physical-link behavior. The subsequent owner run above confirms the normal
ON/SLEEP verification path on hardware; deadline/failure edge cases remain
validated by host tests only.

**These changes improve verification and reporting, not a proven fix for HCI
0x3e.** Longer-running board/Echo tests remain open.

## Echo verification timing and approved target window: 0.2.6

The owner's 0.2.5 retest still produces the spoken warning. Logs show accepted
PUTs with unsupported=0 and handler times of 6–16 ms. Echo immediately queries
all addressed lights repeatedly, receiving the old opposite `on` value with
reachable=true and pending=true. BLE completes seconds later. OFF addresses both
the virtual group and the individual stations. The evidence points to immediate
state verification rather than a missing command or slow handler; it does not
expose Alexa's internal decision algorithm or full network latency.

The owner explicitly chose temporary target reporting to Alexa, with measured
states retained in the dashboard/local API. Version 0.2.6 reports the accepted
ON/SLEEP goal in Hue `on` only while its command is pending/busy and within
30000 ms of acceptance. Completion, final error or expiry restores measurements.
Reachability is never forced true, observed data/timestamps are never changed by
projection, and there is no projection after reboot or without a command. Group
ON aggregates individual Hue views. Logs identify which view contributed.

Identical pending/busy goals are coalesced without restarting a job, generation
or acceptance time. Different goals supersede older ones; a new request after
completion can retry. This avoids redundant group-plus-individual work without
creating an indefinite target window. Native core/Hue tests cover both targets,
active/queued work, expiry, completion without confirmation, failure, reversal,
retries after completion, duplicate deadlines, millis wrap, missing measurements,
measured reachability and the group/individual duplicate sequence.

The firmware build, native core/Hue suites, seven Python translation/layout/NVS
checks and browser/USB suites passed locally. Browser tests retain real measured
badges when a target is pending and exercise English/German policy copy. No
attached Echo or ESP32 was used for those checks.

**This is an owner-approved reporting-policy change, not confirmation of physical
switching.** The owner subsequently reports improved Alexa behavior; the follow-up
log above shows the two sources transitioning as intended. This is not complete
validation of every Echo/board condition. BLE transport/retry settings remain those of 0.2.4.

## Owner improvement report and Alexa ACK correction: 0.2.5

The owner reports that 0.2.4 improves reliability, including switching all four
stations, but intermittent failures remain. Alexa sends the commands and the
stations switch, yet Alexa announces a problem. The supplied 0.2.4 log contains
69 successful read jobs, 38 failed worker attempts across reads/commands and
24 sent command reports. One SLEEP job for station 4 explicitly fails after all
three worker attempts; subsequent reads still show ON until a later command.
Thus the excerpt also contains a genuine failed operation. These are log counts,
not a controlled before/after reliability comparison. Raw logs with personal
credentials are not stored in the repository.

Inspection of the pinned Espalexa fallback shows a concrete protocol defect:
every state write is acknowledged as `{"/lights/1/state/":true}`, regardless
of the actual light ID or requested ON/OFF value. Version 0.2.5 intercepts light
PUTs and returns `{"/lights/<actual-id>/state/on":true|false}` in a Hue success
array, after validating and queuing the accepted target. The HTTP handler never
waits for BLE and never changes measured state. Resource parsing and JSON response
generation live in `include/hue_api.h` and are tested directly on the host with
the pinned ArduinoJson dependency. Other Espalexa discovery/registration behavior
and all existing IDs/names remain unchanged.

Native tests cover all four station IDs and the group, both boolean targets,
collection routes, a maximum bridge identity, malformed/foreign/overflow IDs,
invalid/non-object/oversized JSON, wrong value types, empty/overlarge property
sets, unsupported properties including unrelated true values, unconfigured
stations and separation of accepted targets from measured state. CI runs the
same test after installing the firmware dependencies. Invalid power requests
never enqueue; valid `on` can succeed alongside a per-property unsupported error.

Serial PUT/GET traces show target acceptance and handler duration, cached measured
on/reachability and pending work, without request paths, usernames or bodies.
This allows a later Echo test to distinguish an accepted command from a stale,
unavailable or not-yet-changed reading. **The wrong ACK is proven in source; its
role in the owner's spoken Alexa warning is still a hypothesis.** GET reachability
is deliberately not forced true; genuine BLE failures remain visible. A real
Echo/board retest is required before claiming the warning is resolved.

The 0.2.5 firmware build and native core/Hue suites passed locally, as did the
seven Python asset/translation/NVS checks and Node USB/layout checks. The new
Hue tests exercise production parsing/ACK code with a host command queue; they
do not emulate an Echo's spoken-error policy or real HTTP/radio timings.

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

The owner subsequently reports improved switching/read reliability (see the
0.2.5 evidence above), with remaining failures. The upgrade exercises an upstream
recovery path; the physical cause and complete hardware reliability remain
unconfirmed.

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
  dynamic entries up to 16, discovery/address suggestions with fixed Random (1), preservation of edits during polling, save errors, power actions,
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
7. Check Alexa discovery of individual configured lights (no virtual group), voice commands, renaming and
   measured-state queries. An acknowledgement is not confirmation of BLE success,
   and app caching remains controlled by Alexa.
8. Check longer-running heap usage, HTTP responsiveness during BLE work,
   concurrent SteamVR use and recovery after power/network interruptions.

Builds and browser simulations do not replace hardware tests. No board is
attached to this development environment. Owner feedback confirms startup/basic
operation and intermittent BLE read failures, not complete validation of every
feature and edge case.
