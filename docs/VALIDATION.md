# Validation report — 0.3.9

## rs-matter 0.1.19: room label and confirmed maintenance

Owner-approved classic OTA from 0.1.18 to **0.1.19** is installed. One upload
of the published 1,959,216-byte app was accepted after physical BLE idle; the
board rebooted normally and reports the expected version. No separate reset,
restart, room save, station power command or commissioning action was performed.

The rs-only dashboard hides its introduction once an actual station is configured,
including an unreachable station. Clicking the localized room heading opens a
Save/Cancel editor; the private room label is persisted independently of the
hostname. A private Dangerzone shows setup SSID/password with plain-text copy,
Restart and an additional explicit factory-reset confirmation. Full reset removes
all standard NVS settings/fabrics and requires USB key recovery and first setup.

The new native maintenance program executes the production parser, UTF-8
validation, durable reset scheduling/boot sequence and idle/delay gate with fake
storage. It covers malformed and extra fields, missing/non-boolean confirmation,
embedded NUL action, normal boot, each storage failure, millis wrap and no erase
before reboot. This is simulated storage evidence, not physical flash-erasure
or power-loss verification. Review confirms both routes use personal-session and
Origin/JSON checks, and only standard NVS is erased before Wi-Fi/BLE/Rust startup.

The new browser program passes EN/DE at 1280/390/320 with mocked HTTP only:
empty/disabled versus configured/unreachable stations, keyboard edit/cancel,
draft preservation across polling/language changes, persistence/reload, server
failure, safe Unicode/HTML-like text, private credentials/copy styling, cancelled
reset without any request, failed and accepted restart, exactly one confirmed
reset request, obsolete-key clearing and logout. It asserts no power, network,
configuration or commissioning requests and no overflow/page errors.

All four complete local builds fit the unchanged 1,966,080-byte slot: classic
1,959,440 bytes, both C3 profiles 1,949,312, S3 1,884,976. Static RAM is
67,620 / 55,072 / 64,052 bytes respectively; these are not runtime headroom
measurements. Cache-off libc/diagnostic/Wi-Fi wrapper guards pass. Stable 0.3.9
and SDK 0.3.9-matter regression builds pass. Pages assembly and all 34 Python
checks pass, including chip/layout/hash/OTA/source ZIP validation. All 16
browser/USB programs pass against the freshly built Pages assets. All native
CI programs, including six GPIO/OTA chip variants and the new maintenance program,
pass; all eight Rust tests pass in debug and release.

Dangerzone reset/restart behavior, room-save persistence, power interruption and
runtime memory margin remain unverified on hardware. All app slots/partitions are retained.

Public release verification: source commit `f788690`, GitHub Actions
[run 44771254454](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/44771254454)
completed build and Pages deployment successfully. Live EN/DE WebFlasher checks
at 1280/390/320 confirm all four exact manifests/download paths and version,
consent gates, no overflow/page errors and no mutation/upload requests.

All four public packages were downloaded and verified: manifest path SHA256,
chip IDs, USB offsets, exact complete partition table, every SHA256SUMS entry,
OTA payload equality, ZIP CRC/payload and packaged source equality with the
released implementation. Public app bytes: classic **1,959,216**, both C3
profiles **1,949,072**, S3 **1,884,752**. Local values above differ with build
paths. The subsequently owner-approved device update used this published classic
package; no second upload was needed.

Pre/post comparisons retain all four station IDs/names/addresses and OFF choices,
API key, own web-password access, username settings, hostname/setup SSID, network
mode and GPIO bindings. The board reports home Wi-Fi connected, Matter ready and
paired, a usable BLE worker and all four stations reachable. Last-command history
is none on all four after boot. The new room label starts empty as intended.

Real guarded browser checks pass EN/DE at 1280/390/320: actual 0.1.19 UI, four
cards, hidden introduction, localized default room label, editor open/Escape
cancel, visible Dangerzone controls, matching private setup SSID/key, plain-text
copy styling, no overflow/page errors. Only GET and the dashboard’s read-only
refresh requests are allowed by the guard; no power/config/room/maintenance/
commissioning writes occur. The diagnostic personal session is closed. Future
device changes still require a new explicit owner approval.

## rs-matter 0.1.18: GPIO buttons and separate board builds

Owner-approved classic OTA from 0.1.17 to 0.1.18 is installed. The owner now
authorizes publication via the existing main-branch Pages workflow. Requested profiles are classic ESP32, WeAct ESP32-C3FH4,
ESP32 C3 SuperMini and ESP32 S3 SuperMini (owner specifies 4 MB flash and
2 MB on-chip PSRAM; QSPI enabled). Both app slots and the full partition table
remain identical. USB bootloader starts at 0x1000 classic, 0x0000 C3/S3.

All four complete Rust/Arduino builds pass, including chip-specific image IDs,
slot bounds, partition equality, OTA payload/digest and cache-safe libc/allocation
hook/Wi-Fi wrapper checks. Final packaged app bytes / static RAM:

| Profile | App bytes (slot 1,966,080) | Static RAM bytes |
| --- | ---: | ---: |
| Classic ESP32 | 1,954,768 | 67,596 |
| WeAct ESP32-C3FH4 | 1,945,024 | 55,048 |
| ESP32 C3 SuperMini | 1,945,024 | 55,048 |
| ESP32 S3 SuperMini | 1,880,016 | 64,036 |

Static sizes are not runtime margins. Classic ELF retains the 14-KiB arena,
52,528-byte Rust Stack and 18-KiB foreground task. The C3 allocation hook and its
SDK atomic add/CAS callees resolve to internal IRAM. SDK defaults are overridden
consistently after sdkconfig for one client/central+observer NimBLE host on every
C/C++ translation unit; no second BLE owner or Rust polling task is introduced.
C3 portable atomics use the existing critical sections; its logger registration
runs only on the foreground Rust initialization path. Dependency versions stay
locked; only a target-specific feature/dependency edge was added.

Twenty native programs pass, including GPIO and OTA validation for all three
chip families: held-at-boot, initial-sample bounce, stable release-to-arm,
40-ms debounce, no hold repeats, next press, millis rollover, unsupported/duplicate
pins and wrong-chip images. GPIO JSON is canonicalized before one atomic NVS
write, so extra accepted fields cannot enlarge the saved record. Invalid saved
bindings fail disabled. Buttons enqueue existing commands and respect saved OFF
preferences; no startup, settings-save or failed-maintenance press sends power.

All eight release Rust tests pass, including encrypted Descriptor reads,
endpoint identity/aggregator paths and no startup writes. Stable 0.3.9 and SDK
0.3.9-matter builds also pass. All 34 Python checks and 15 browser/USB programs
pass. New UI coverage uses real generated templates and mock APIs at EN/DE
1280/390/320: explicit private settings fetch, GPIO/action edits, language
preservation, one save, logout clearing and no implicit power/network mutation.
Board selection chooses exact manifests/downloads, resets consent and rejects
mismatched chip families. Existing OTA reconnect tests use distinct synthetic
future versions to preserve their external-update scenario. Local Pages assembly
validates every board's actual parts/hash/table/OTA package.

Owner-approved actual classic OTA succeeded with one upload. The preflight
verified the target, old version and matching classic package/source fingerprint.
Pre/post comparisons retain four assignments and OFF preferences, API key,
own password, username settings, hostname/setup name and network mode. The board
reports 0.1.18, home Wi-Fi connected, Matter ready/paired and usable BLE. Both
new GPIO bindings are disabled. A live guarded Chromium pass verifies actual
EN/DE home/GPIO UI at 1280/390/320 without overflow or page errors, and logs out.
No power, configuration, setup or commissioning action was sent. The first
verifier attempted plain-text decoding of gzip HTML after its successful settings
comparisons; read-only browser checks resolved this diagnostic error. There was
no second upload. Sanitized evidence is .pio/rs-0118-live-{ota,browser}.log;
credentials and device identifiers are not copied into documentation.

Public release verification: [GitHub Actions run 43707555895](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/43707555895)
completed build and deployment successfully for commit 38860c3. Actual public
main-page notice, four-board manifests/download links and consent reset pass in
Chromium at EN/DE 1280/390/320, with no page errors or upload requests. All four
packages were downloaded and checked for chip IDs, USB offsets, complete
partition equality, manifest hashes, every image/OTA checksum, OTA payload,
ZIP integrity/payloads and source matching the release commit. Public CI app
sizes are 1,954,544 classic, 1,944,800 both C3 profiles and 1,879,792 S3; the local
build figures above differ with build paths. Sanitized public checks are retained
in .pio/rs-0118-public-{files,browser}.log. No additional device action occurred.

That is classic OTA/home-UI evidence only. Physical GPIO contacts, C3/S3 startup/RAM,
USB diagnostics, BLE/coexistence, home/local mode, OTA and Alexa pairing still
need hardware validation. See the
[GPIO and board guide](GPIO-BOARDS.md).

## rs-matter 0.1.17: setup portal presentation

The owner confirms successful Android setup access after 0.1.16 and shows the
portal landing page. This proves setup connection/rendering, not permanent Local
Mode/BLE operation. Do not copy its real SSID/home network into fixtures or previews.

The rs-only setup root now uses local HTML/CSS with home/local choice cards,
clear descriptions, a broadcast-SSID panel, consistent gaps and smaller secondary
Info/Leave setup actions. The misleading expected home-disconnected status is
removed from the landing page; vendor form/status/error behavior is retained.
Confirmation and restart pages share the theme and honor the language selected
at the root. Only the language preference is stored, never any credential.
Handlers/routes/clean setup/nonce checks and existing raw-BIN blocks are preserved.
A narrow derived manager forwards the pinned vendor captive redirect and access-
timestamp hooks before rendering, preserving the original root request behavior.
No radio/auth/NVS/mode/identity behavior is changed.

All thirteen browser/USB programs and all 31 Python checks pass. The real local
root/confirmation/restart templates are exercised in EN/DE at 1280/390/320:
visible card/action gaps, >=44px secondary targets, no horizontal overflow,
selected language carried through the flow and GET-only navigation to existing
routes. Preview PNGs under .pio/rs-0117-portal-*.png use synthetic names only;
the real screenshot/SSID/home network is not copied into source or previews.

Final app 1,950,896/1,966,080 bytes; static RAM 67,580. Exact partition/chip/OTA/
app/hash/cache-safe libc/Wi-Fi guards pass. ELF retains 14 KiB arena, 52,528-byte
Stack and 18 KiB foreground task. Archived 0.1.16 comparison confirms Rust changes
only version metadata; radio/protocol/auth/config/identity helpers and the base
dashboard are unchanged. Reviewing the production source diff confirms only
setup rendering changes. Runtime memory is not inferred from linker figures.

Owner-approved real OTA from 0.1.16 succeeds. The board reports 0.1.17, home
mode, Wi-Fi connected, Matter ready/paired, usable BLE and no startup error.
Four assignments and OFF choices, API key, own password, username settings and
naming match pre-update values. A post-boot sample at 4073 ms reports 26,036 free
data-heap bytes and a 24,564-byte largest block; it is not a guaranteed margin.

Real guarded Chromium passes EN/DE home layouts at 1280/390/320: no overflow,
retained display/debug behavior, enabled controls and ready/paired Matter panel.
No setup/mode/power/config/commissioning request or page error occurs. Diagnostic
sessions are closed. The agent did not activate the new setup portal; the owner
subsequently confirms the 0.1.17 portal works on the device. Permanent Local
Mode/BLE operation and long-run reliability still need hardware checks.

## rs-matter 0.1.16: clean setup boot after Android connection rejection

Owner hardware evidence for 0.1.15 setup is negative: Android rejects the AP
password even after forgetting the network. Read-only HTTP checks confirm the
supplied key authenticates and the bridge's setup SSID matches. Permanent Local
Mode was not reached. No raw keys/SSIDs/responses are stored in this report.
The root cause remains open; password rejection does not prove the key is wrong.

Candidate setup uses a one-shot flag, waits for physical radio idle and reboots
into AP-only setup before initializing Matter or BLE. The flag is cleared before
Wi-Fi starts and does not change the saved mode or other NVS records. Clear/write
failure leaves a usable saved-mode path. Portal close/timeout reboots normally;
its existing home/local selection remains explicit. Browser handoff retains the
key for copying and stops polling. This removes initialized stack/heap contention
from setup, without establishing that contention caused the hardware symptom.

Physical-USB LH_WIFI_INFO and main-loop AP event logs expose numeric configuration
and event information, match booleans and heap values, without keys, SSIDs or MACs.
Event context uses atomic numbers only. Permanent Local Mode radio work is unchanged.
Espressif's [ESP32 coexistence guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-guides/coexist.html)
classifies SoftAP connecting/connected with BLE as C1 (supported but unstable).

All 18 native programs pass. Mode tests cover exactly-once setup enqueue/consume,
failed enqueue/clear, subsequent timeout/reboot semantics and preservation of the
saved mode/unrelated settings. Eight Rust tests pass in debug/release, 31 Python
checks and all thirteen browser/USB programs pass. New EN/DE setup-handoff tests
cover confirmation/cancel/save failure, restart instructions, copyable API key
across rebooted 401s, stopped polling and no implicit power/mode requests.

Final rs app: 1,942,832/1,966,080 bytes (+2,096 versus 0.1.15); static RAM 67,588
(+16). Partition/chip/OTA/app/hash/cache-safe libc/Wi-Fi guards pass. ELF retains
14 KiB arena, 52,528-byte Stack and 18 KiB task. Stable/SDK builds pass; the SDK
app remains 1,963,792 bytes and 123,284 static RAM bytes. Archived 0.1.15 comparison
confirms unchanged Rust protocol/transport, station/state/config/identity helpers,
base HTML and dependency lock after normalizing root version metadata.

Owner-approved real OTA from 0.1.15 succeeds. The board reports 0.1.16, home
mode, Wi-Fi connected, Matter ready/paired, usable BLE and no startup error.
Four assignments and OFF choices, API key, own password, username settings and
naming match pre-update values. A post-boot sample at 4071 ms reports 22,040 free
data-heap bytes and a 20,468-byte largest block; this is not a guaranteed margin.

Real guarded Chromium passes EN/DE at 1280/390/320: no overflow, home-mode
controls, retained display/debug behavior, ready/paired Matter panel and new
clean-setup restart confirmation in both languages. Both confirmations were
cancelled; no setup/mode/power/config/commissioning POST or page error occurred.
Diagnostic sessions are closed. The actual isolated setup was not activated;
Android connectivity and AP/BLE operation need owner hardware retests. The
original password-rejection cause is still not proven.

The owner subsequently confirms Android connects successfully and supplies the
setup-root screenshot. This positive result covers setup access/display; no
permanent Local Mode/BLE or prolonged coexistence result follows from it.

## rs-matter 0.1.15: permanent Local Wi-Fi Mode

The rs-only setup menu provides a separate token/Origin-protected confirmation
POST for permanent Local Mode. One atomic wifi_mode record selects home/local;
old records default home. It does not rewrite router credentials, web auth,
API key, stations, endpoint registry or fabrics. Boot starts a protected AP at
192.168.4.1 and the dashboard/sequential BLE worker without the Matter runtime.
Registry checkpointing and measured BLE behavior remain intact. No automatic
power command is introduced. Returning home resumes normal Matter startup.

The UI distinguishes local Wi-Fi from a lost home connection, keeps BLE and OTA
controls usable, explains paused Matter without implying lost pairings, and offers
confirmed return to saved home Wi-Fi. AP key/name changes restart safely; the new
key stays visible/copyable across a rebooted 401 and clears on explicit logout.
USB reports the local address with an explicit network_mode field while preserving
strict HTTP IPv4 URL validation. USB setup copy is translated in EN/DE.

All 18 native programs pass. The mode-storage test verifies old/unknown defaults,
exact local selection, failed persistence and untouched unrelated settings. All
eight Rust tests pass in debug/release, 31 Python checks and twelve browser/USB
programs pass. New EN/DE mobile-emulated tests use the real portal/dashboard HTML
and cover token form submission without URL secrets, local controls with paused
Matter, return-home cancel/failure/restart, key/name handoff and new-key retention
across rebooted 401s, copying and logout. USB frames accept local URLs only with
explicit local mode while retaining strict URL validation, and USB copy explains
LOCAL_MODE. These are simulated network/backend tests, not actual AP operation.

Final app 1,940,736/1,966,080 bytes (+10,384 versus 0.1.14), static RAM 67,572
(+32). Exact partition/chip/OTA/app/hash/cache-safe libc/Wi-Fi guards pass. ELF
retains 14 KiB arena, 52,528-byte Stack and 18 KiB task. Archived 0.1.14 comparison
confirms Rust protocol/transport and BLE/state/config/identity helpers unchanged
after normalizing version metadata. Cargo.lock changes only the root package.
Stable PlatformIO and the SDK prototype also build successfully; the SDK app is
1,963,792/1,966,080 bytes with 123,284 static RAM bytes.

Owner-approved real OTA from 0.1.14 succeeds. The board reports 0.1.15,
network_mode:home, Wi-Fi connected, Matter ready/paired, usable BLE and no startup
error. Four assignments and OFF choices, API key, own password, optional username
settings and custom naming match pre-update values. A post-boot sample at 4244 ms
reports 26,556 free data-heap bytes and a 25,588-byte largest block; this is not
a guaranteed margin. Diagnostic sessions are closed.

Real guarded Chromium checks pass EN/DE at 1280/390/320: retained theme/colors,
right-side status, matching language/Sign out dimensions, no overflow, enabled BLE
controls, Local Mode setup help, return-home action hidden while in home mode,
debug-only refresh and ready/paired Matter information. No power/config/network-
switch/commissioning request or page error occurs. The hardware was left in home
mode; no AP activation, credential change or pairing action was performed.
Actual AP/BLE coexistence, AP OTA, phone connectivity, AP credential persistence
and home/AP switching remain hardware checks. Alexa app behavior after such a
round-trip has not been verified.

## rs-matter 0.1.14: revert language selector colors

Remove only the black-on-light selector/options override from 0.1.13, restoring
its preceding theme-native appearance. Preserve control dimensions, debug-only
manual refresh and automatic background reads. No radio/auth/config/identity
change. Both targeted account/preferences and presentation/OTA-reload browser
programs pass, including preceding computed theme colors and unchanged control
sizes, debug-only refresh and no read on debug toggle. All 31 Python checks pass.
App 1,930,352/1,966,080 bytes, static RAM 67,540 unchanged. Exact partitions/chip,
OTA/app/hash, libc and Wi-Fi guards pass; ELF arena/Stack/task remain
14 KiB / 52,528 bytes / 18 KiB.

Owner-approved OTA from 0.1.13 succeeds. The board reports 0.1.14, Matter ready/
paired, usable BLE and no startup error. Four assignments, OFF choices, credentials
and naming match pre-update values. A post-boot sample at 4020 ms reports 26,732
free data-heap bytes and a 25,588-byte largest block; it is not a guaranteed margin.
Real guarded Chromium checks pass EN/DE at 1280/390/320: preceding language colors,
matching control dimensions, no overflow, refresh hidden normally and visible in
debug across language changes, and no extra read on debug toggle. No power/config/
commissioning request or page error occurs. Diagnostic sessions are closed.

## rs-matter 0.1.13: language contrast and debug-only manual refresh

The rs-generated dashboard sets black language text/options on a light background
and uses the light native control scheme while retaining existing dimensions.
Read current states is hidden initially and follows debug visibility, including
language changes and logout. Toggling debug does not request a BLE read; existing
startup/periodic read scheduling is unchanged. Only UI/version metadata changes.
Three targeted browser programs pass: account/preferences, presentation/OTA reload
and the rs-matter gzip dashboard. Assertions cover black selected/option text on
a light background, preserved dimensions, refresh hidden by default/shown in
debug and no extra read on debug toggling. All 31 Python checks pass. Final app:
1,930,368/1,966,080 bytes (+32 versus 0.1.12), static RAM unchanged at 67,540.
Partition/chip/OTA/hash/cache-safe libc/Wi-Fi guards pass; ELF arena/Stack/task
remain 14 KiB / 52,528 bytes / 18 KiB.

The separately owner-approved OTA from 0.1.12 succeeds. The board reports 0.1.13,
Matter ready/paired, usable BLE and no startup error. Four assignments and OFF
choices, credentials and naming match the pre-update snapshot. A post-boot sample
at 4052 ms reports 26,276 free data-heap bytes and a 24,564-byte largest block;
this sample is not a guaranteed margin. Real guarded Chromium checks pass EN/DE
at 1280/390/320: black selected/option text on light background, equal language
and Sign out dimensions, no overflow, refresh hidden normally and visible only
in debug, preserved across language changes, and no extra read on debug toggle.
No power/configuration/commissioning requests or page errors occur. Diagnostic
sessions are closed. Long-term and sixteen-station operation remain unverified.

## rs-matter 0.1.12: account controls, OFF preferences and responsive cards

The implementation adds a persisted per-station sleep/standby OFF choice,
used by rs-matter OFF and toggle-off and the dashboard's individual/all OFF.
Explicit local sleep/standby commands remain exact. Missing stored fields default
to sleep; omitted update fields retain existing choices. STANDBY requires measured
raw 2 to confirm, preserving measurement history and the bounded read-only window.
Stable/SDK firmware retains SLEEP-only OFF behavior. Identity/partition/worker
changes and automatic power writes are excluded.

Optional username settings persist in the same web_auth record as salt/hash.
First setup ignores usernames; old accounts remain password-only. Personal-session
settings can enable/disable/rename, invalidate other sessions and preserve the
password. Public login capability exposes only the enabled flag. Confirmed API-key
regeneration persists access_key without changing router Wi-Fi, web password,
station or fabric data. These routes reject API-key/bootstrap access and use
existing Origin/JSON guards. UI secrets remain memory-only and clear on logout.

The rs-only UI places status right of branding, matches the language selector to
Sign out, corrects English terminology and puts the Alexa name first. Debug reveals
station ID/MAC/history and expands cards; normal mode hides those details and
shrinks cards, retaining pending/unavailable feedback. Mobile native discovery
selection coexists with manual input and preserves edits across language/status
updates.

All 17 native test programs pass, including maximum-size 16-station OFF config
stream/restore, omitted-field migration, exact standby confirmation and Matter
OFF dispatch. Legacy auth missing/null/non-string username fields load empty;
strict username validation rejects invalid identifiers. Eight Rust tests pass in
debug/release, 31 Python tests and all eleven browser/USB programs pass. New EN/DE
mobile-emulated tests cover enable/rename/disable/sign-in, failed name saves,
confirmed/cancelled/failed key rotation and draft preservation. Stable PlatformIO
build passes. The final app is 1,930,336/1,966,080 bytes with 67,540 static RAM bytes
(+16 versus 0.1.11). Exact partitions/chip/OTA/hash/cache-safe libc/Wi-Fi guards
pass. ELF keeps 14 KiB arena, 52,528-byte Stack and 18 KiB foreground task.

Owner-approved real OTA to the final app succeeded. The board reports 0.1.12,
Matter ready/paired, usable BLE worker and no startup error. Four assignment
IDs/names/MACs, API key, web password and custom naming match pre-update values;
all OFF defaults are sleep and command histories none. Optional username is off
and the private name empty. A post-boot sample at 9223 ms reports 24,892 free data
heap bytes and a 22,516-byte largest block; this is not a guaranteed margin.
Real wrong-Origin and API-key-only POSTs to the new account mutation routes are
rejected. These negative checks did not change settings; sessions were closed.

A real guarded Chromium tab initially loaded 0.1.11 and automatically reloaded
0.1.12 after external OTA. EN/DE 1280/390/320 checks confirm right-side status,
equal language/Sign out sizing, expanding/shrinking debug cards, hidden normal
ID/MAC, available OFF/native-select/account controls and paired/closed Matter
status. No forbidden power/config/commissioning request or page error occurred.
Actual standby writes and username/key changes, individual phone browsers,
long-term and sixteen-station operation remain hardware checks. The own-upload
reconnect flow is covered by software; real external-update reload is verified.

## rs-matter 0.1.11: name, measured colors and OTA reload

The rs-only dashboard moves the connection pill left of the top branding and
uses measured-state colors: existing green ON, gray Unknown, blue SLEEP and
orange STANDBY. Unreachable cached readings use Unknown gray; pending targets do
not change measured colors. The label remains explicit, contrast is checked,
and card/status geometry is retained. Display/product/commissioning name becomes
Lighthouse-Bridge without altering saved station names, IDs or fabrics. The test
attestation material is unchanged.

Browser OTA reload starts only after the receiver acknowledges installation.
It tolerates old authenticated responses and temporary downtime, and reloads
once the rebooted login gate responds. Waiting is bounded at 90 seconds; no
automatic re-upload occurs. External updates are recognized through status
version mismatch or a changed public page version after session loss. Ordinary
expiry returns to login without reloading unchanged firmware. Older already-loaded
HTML cannot gain this behavior until refreshed once.

Targeted Chromium scenarios pass EN/DE at 1280, 390 and 320 pixels without
horizontal overflow, verify badge foreground/background contrast >=4.5 and
measured/pending-state separation, and simulate rejected upload, successful
upload, old server, downtime, reboot, external update and unchanged session
expiry. These are mock-backend software checks, not a physical OTA/reload test.
The separately owner-approved physical OTA from 0.1.10 succeeded. The board
reports 0.1.11, Matter ready/paired, BLE worker available and no startup error.
Four assignments, credentials and custom naming match pre-update values; command
histories remain none. A post-boot sample at 4378 ms reports 26,684 free data-heap
bytes and a 25,588-byte largest block; this is a sample, not a guaranteed margin.
Real Chromium checks pass EN/DE at 1280/390/320 px: connection status left of
branding, no horizontal overflow and all four measured SLEEP badges blue.
Explicitly opening Matter Setup reads the private status, displays Lighthouse-Bridge
and preserves paired/closed-window state with no code. A request guard confirms
no power/config/commissioning changes; diagnostic sessions are closed. Other
measured colors and the new OTA reload flow remain software-tested: the old
0.1.10 tab must first be refreshed to obtain the new HTML.

All eight Rust tests pass in debug/release, all 31 Python checks and all ten
browser/USB programs pass. The new preferences scenarios also cover unchanged
session expiry without reload and authenticated version mismatch with exactly
one reload, keeping language and never re-uploading. Foreground task/arena ELF
checks retain 18 KiB task, 14 KiB arena and 52,528-byte Stack. Exact partition,
classic chip, OTA/app/hash equality, cache-safe libc and Wi-Fi-wrapper guards
pass. The final app is 1,925,824/1,966,080 bytes (+480 over 0.1.10), with unchanged
67,524 static RAM bytes. Comparing archived 0.1.10 sources confirms Rust logic,
shared C++/base HTML and transport are unchanged after normalizing Rust display
name/version metadata. Long-term and sixteen-station operation remain unverified.

## rs-matter 0.1.10: Matter Setup without a separate show button

The real generated gzip dashboard loads personal-session status/active code
when Matter Setup is explicitly opened. The separate show button is absent;
only the new-code/window action sends a commissioning POST. Login and a collapsed
panel do not retrieve a code. Closing clears private display and stops polling;
late responses cannot restore a closed panel's code. Reopening queues a fresh
read behind an existing request without rotating credentials. Logout closes the
section, and relogin requires another explicit opening. Preserve active-code
copy, language switching, queued renewal, polling stability, readable startup
errors and firmware-recovery controls.

The classic app is 1,925,344/1,966,080 bytes, 64 bytes above the prior
0.1.9 image; static RAM remains 67,524 bytes. Build guards preserve exact partitions,
classic chip, OTA/app equality, cache-safe libc, Wi-Fi wrapper and future arena.
There are no Rust/BLE/network/endpoint changes apart from version metadata.
Targeted generated-dashboard browser scenarios pass in English and German.
The expanded scenarios additionally verify stale timer suppression while closed
and an explicit renewal queued behind a background fetch completing even after
the user collapses the section, without revealing its code until reopening.
All eight Rust tests pass in debug/release, all 31 Python package checks and all
nine browser/USB programs pass. ELF checks retain a 14 KiB arena, 52,528-byte
Stack object and 18 KiB task stack. Comparing against the archived hardware-tested
0.1.9 sources confirms that Rust/C++ protocol, BLE, API, shared base HTML and
adapter sources are unchanged after normalizing version metadata only.
The owner separately approved physical OTA to 0.1.10. It returned installed and
restarting, then reported 0.1.10, Matter ready/paired, BLE worker available and no
startup error. Four assignments, API key, personal password and naming matched
the authenticated pre-update snapshot. All command histories remained none.
One post-boot sample at 4431 ms showed free byte heap 26684/largest 24564; this is
not proof of safe runtime margins.

Actual headless Chromium on the real board verifies the new section in English
and German: the show button is absent, a closed section/login makes no private
code request, opening/reopening fetches ready/paired/closed-window status directly,
and collapse clears the view. The new-code control has its correct EN/DE label;
the code stays hidden without an active window. A network guard confirms zero
commissioning, power, configuration or firmware requests during these UI checks.
The diagnostic browser session signed out. No new pairing window was opened;
active-code renewal/error handling remains covered by software scenarios rather
than a new hardware commissioning attempt. General code cleanup is deferred.

## rs-matter 0.1.9: missing Alexa endpoint 1

Direct authenticated LAN inspection of the deployed 0.1.8 firmware confirms four
configured stations and ready/paired=true. An actual RustCrypto PASE handshake
and encrypted read-only Descriptor probe obtains root PartsList `[2,3,4,5,6]`,
aggregator-2 PartsList `[3,4,5,6]`, and UnsupportedEndpoint for endpoint 1's
DeviceTypeList and PartsList. Root type is 0x16/revision 4, aggregator type
0x0e/revision 2, and all four station types 0x100/revision 3 plus 0x13/revision 3.
This supersedes the owner's earlier manually reported inventory including 1.
Assignments were unchanged; this is not an SDK iterator losing an existing 1.

The diagnostic client only reads Descriptor attributes and closes its temporary
PASE session with Secure Channel CloseSession. Afterward Matter remains ready
and paired; uptime increases and all four command histories remain none. It
never sends AddNOC, RemoveFabric, station writes or firmware updates. Web session
and setup code are used privately; raw credentials and device identities are not
included in repository evidence. No USB or Echo app is attached to the environment.

The [matter.js ecosystem notes](https://github.com/matter-js/matter.js/blob/main/docs/ECOSYSTEMS.md)
document Alexa's additional endpoint-1 requirement. The deployed topology violates
that compatibility requirement and reproduces the exact logged rejection.
This establishes a concrete incompatibility, not proof that no other GS014 cause
can remain after correction.

The candidate conditionally supplies aggregator 1, retaining empty aggregator 2
and every active station endpoint/UniqueID. A custom PartsMatcher assigns every
station to exactly one aggregator and excludes aggregator endpoints from child
lists. A legacy station at 1 retains its light type and original aggregator 2.
No registry/fabric reset or renumbering is performed. The owner explicitly
approved this narrowly documented retired-endpoint-1 policy exception and OTA,
while requiring a fresh question before future device changes.

Before the fix, the new isolated missing-one fixture fails on encrypted CASE
read `1/29/0` with UnsupportedEndpoint, matching hardware. Afterward tests cover
this four-station case, maximum sixteen stations without 1 (19 metadata endpoints),
legacy-light configurations, zero stations, sparse/shuffled IDs, actual serialized
DeviceTypeList/PartsList/ServerList and chunked scalar reports with success ACKs.
Seeded CASE fixtures do not test handshake establishment. Actual OTA, real PASE
topology verification and subsequent owner-confirmed Alexa pairing are recorded
separately below.

Local candidate verification: all eight Rust host tests pass in debug and
release, all sixteen native C++ programs, 31 Python checks and nine browser/USB
simulations pass. The final classic app is 1,925,280/1,966,080 bytes and static RAM
67,524 bytes. Exact partition-table equality, classic chip ID, OTA/app identity,
SHA256, cache-safe libc, diagnostics and Wi-Fi wrapper checks pass. ELF getters
retain the 14 KiB arena and 52,528-byte Stack object; app_main still passes 18432
to task creation. The exact future-size/alignment + 1 KiB guard passes.
Descriptor DataVersions are now randomized once per boot instead of fixed at 1,
so an existing controller's old version filter cannot silently retain the prior
aggregator PartsList.

The approved real OTA from 0.1.8 to 0.1.9 returned installed/restarting=true.
After reboot the device reports 0.1.9, ready/paired=true, BLE worker available
and no startup error. The pre/post authenticated snapshots preserve all four
station IDs/names/MAC assignments, API key and device naming; the same personal
password still authenticates. All four command histories remain none. A single
post-boot sample at 4337 ms shows byte heap 26584/largest 25588, not proof of safe
long-term or commissioning memory margins. This establishes one actual OTA and
recovery on one classic board, not power-loss/rollback or 16-station validation.
The diagnostic web session was closed. The owner separately approved opening a
fresh 15-minute commissioning window and direct endpoint checks, requesting its
setup code privately. Actual encrypted PASE Descriptor reads on 0.1.9 confirm
root PartsList [1,2,3,4,5,6], aggregator 1 (0x0e/revision 2) with PartsList
[3,4,5,6], retained aggregator 2 (same type) with empty PartsList, and the same
light/bridged types at 3..6. All reads succeed without UnsupportedEndpoint.
Readiness/pairing stay true, uptime increases, and all command histories remain
none. The probe closes its own PASE/web sessions, leaving the approved pairing
window open with its unchanged setup code. No code or credentials are stored
in repository evidence. The owner subsequently confirms complete Alexa pairing
with 0.1.9. This is positive four-station owner evidence, not prolonged or
sixteen-station operation.

At the owner's explicit request, a real headless Chromium session logged into
the actual deployed dashboard and clicked All to SLEEP once for a synchronization
test. All four local IDs 1..4 initially measured steady ON/raw 11 with reachable
true. The real UI request was accepted and the sequential BLE work completed:
every station then reported measured SLEEP/raw 0, reachable=true,
command_status=confirmed, pending=false and busy=false. The browser signed out;
no configuration, firmware or fabric mutation accompanied the power request.
This validates actual web-control-to-BLE operation on four stations. The Alexa
app's displayed states for this specific transition have not yet been reported.

## Independent rs-matter prototype 0.1.8-rs-matter

The owner confirms Alexa GS014 still occurs with 0.1.7. The supplied log again
establishes a CASE session and successful processing/persistence of
CommissioningComplete, followed by UnsupportedEndpoint. UnsupportedCommand and
UnsupportedCluster appear earlier. Their bare labels do not establish whether
they are fatal or optional controller probes; the logged request path is absent.
No allocation failure is reported. UDP has four temporary retries but no hard
errors/timeouts; heap minimum is 3476 and foreground watermark 692 bytes.
These values are not evidence of sufficient stack/RAM margins or an OOM cause.

0.1.8 emits the actual sorted metadata endpoint inventory once at startup and
numeric paths alongside SDK rejection labels. The bounded filter recognizes
only the pinned SDK's typed AttrStatus/AttrPath or CmdStatus/CmdPath Debug
layout. Output is limited to endpoint, cluster and attribute/command ID, with
operation 1=read, 2=write, 3=command; 0xffffffff denotes a missing/wildcard field.
Node/fabric identifiers and arbitrary log text are excluded. Malformed, signed,
truncated or overflowing numeric fields fail closed. The formatter remains
256 bytes and the queue holds 32 numeric records of at most 16 bytes each.
Output, including the startup inventory, occurs after polling and outside locks.
No SDK patch, new protocol acknowledgement, stack reduction or reset is added.

This is a diagnostic release, not a confirmed GS014 correction. Successful
server-side CommissioningComplete processing is not proof of Alexa app acceptance.

The owner measured only 692 bytes left of the 16 KiB foreground task stack.
0.1.8 shifts 2 KiB from the independently allocated Rust future arena to the CPU
task stack: 14 KiB arena + 18 KiB task, preserving the former 32 KiB total for
these two reservations. The exact 12664-byte future must still fit with alignment
and a 1024-byte reserve at compile time. No protocol/station/peer/buffer limit or
BLE stack is reduced; the 64-bit host arena stays 24 KiB. Startup logs include
loop_stack_bytes. Additional numeric diagnostics have a small static RAM cost;
the rebalance is not a claim of sufficient measured stack/RAM margins.

The 0.1.8 baseline verification passed six Rust host tests in debug and release profiles.
Real typed SDK AttrStatus/CmdStatus fixtures produce only their expected numeric
paths; synthetic 64-bit node identities are omitted. Malformed/overflow/truncated
fields and unrelated bodies are rejected. Mock FFI checks inventory/path output
outside critical sections, and startup inventory matches actual sorted metadata.
The classic app is 1,924,864/1,966,080 bytes, static RAM 67,524 bytes. Compiled ELF
getters confirm a 14 KiB arena and a 52,528-byte Stack object; app_main passes
18432 to xTaskCreateUniversal (task stack), and the exact-future compile guard
passes. These are build/software checks, not physical Echo or OTA validation.

## rs-matter endpoint lookup correction 0.1.7 evidence

The owner's 0.1.6 trace establishes a successful protocol-level commissioning:
CASE becomes active, CommissioningComplete is requested and persisted, the
fail-safe disarms and the window closes. A RemoveFabric request removes the old
fabric before the fresh attempt. An UnsupportedEndpoint read follows completion.
UnsupportedCommand and UnsupportedCluster probes also occur during setup, but
none of these error labels alone establishes the cause of Alexa's app result.
Allocation-failure count stays zero; foreground stack watermark reaches 788 bytes.
This is not a claim of adequate stack/RAM reserve or working light controls.

The application supplied metadata as root 0, aggregator 2, then station 1 and
later station IDs. That violates the pinned SDK's documented Node invariant.
Its IM path expander resumes by binary_search_by_key; the old order can make a
configured endpoint disappear during a read. The new regression builds real
ReadRequest TLV, parses it and calls the SDK's expand_read, rather than directly
calling Model::index or Node::endpoint. The legacy production model fails the
SDK's sorted-endpoint debug assertion. A negative legacy-order fixture also
checks that release-mode wildcard expansion silently omits endpoint 1. Corrected metadata sorts
in place without changing persisted IDs or the separate station-indexed arrays.
Tests exercise direct and wildcard Descriptor paths for full, sparse, shuffled
and empty models in both debug and release host profiles; CI runs both profiles.
Previous direct-lookup/construction-order tests did not exercise the IM expander.

Physical Alexa acceptance after the correction remains open. Owner evidence now
supports CASE and protocol commissioning, not Alexa device enumeration, power
control, external-state synchronization, OTA or long-term runtime margins.

All five real-Rust host tests pass in debug and release modes, including the
real SDK Descriptor-read regression and the legacy-order negative control.
Local classic-ESP32 app size is 1,921,968/1,966,080 bytes, static RAM 67,076.
The arena stays 16 KiB, Stack object 54,576 bytes, foreground stack 16 KiB and
BLE stack 8 KiB. These sizes are not runtime headroom or hardware pairing proof.

## rs-matter commissioning diagnostics 0.1.6 evidence

Owner 0.1.5 evidence now confirms mDNS and Matter receive progress. One attempt
reaches has_fabrics=true while the window is still open and Alexa reports GS014.
A fresh excerpt without a version header begins with a retained fabric and closed
window, then reports
has_fabrics=false with an open window. No allocator failure or explicit panic is
shown in that excerpt; minimum data heap is 11412 bytes. The supplied counters
do not identify a RemoveFabric request, failsafe rollback or secure-session error.
Neither a RAM cause nor completed commissioning can be inferred from them.

0.1.6 enables the pinned stack's log facade through a bounded filter. Debug and
trace are compiled out. At most 256 bytes of SDK text are formatted locally;
only fixed allowlisted event/error labels enter a 32-record numeric queue and
cross FFI. Overflow is explicitly reported. Raw text, payloads, certificates,
keys and peer/fabric identities never reach serial through this path. The queue
is drained only after protocol polling, outside SDK/queue critical sections,
using the ROM formatter. Public SDK state logs changes in fabric count, active
PASE/CASE sessions, failsafe state and window state. No SDK source patch, private
cast, stack reduction or protocol-validation bypass is introduced.

This is a diagnostic release. Alexa GS014, physical network interoperability and
OTA remain unverified; software checks must not be described as an Echo test.

Local 0.1.6 checks: five real-Rust host tests, all sixteen native C++ programs,
31 Python checks and all nine browser/USB simulations pass. The real SDK window-open
event reaches the safe output; synthetic confidential text is discarded,
unknown/oversized messages remain bounded, a 40-record burst produces 32 queued
records plus an explicit overflow report, and mock FFI asserts that serial output
occurs outside critical sections. The classic ESP32 app is 1,919,536/1,966,080
bytes, static RAM 67,076 bytes; the Rust arena is still 16 KiB and the Stack
object is still 54,576 bytes. These are software/linker checks, not measured
runtime RAM or pairing performance. Log formatting adds bounded CPU/stack work.

Owner 0.1.4 evidence confirms the configured Wi-Fi profile (static RX 4,
dynamic RX 8, dynamic TX 4, Block Ack 6), one IPv6 address, no explicit panic,
and four initial mDNS sends followed by tx=4/rx=0 with mdns_rx=0 throughout the
open window. Heap minima still reach 192/236 bytes and BLE_NO_MEMORY recurs.
Neither queue bounds nor a locally open window establish Echo discovery.

The adapter violated the public NetworkReceive::wait_available contract: a
fixed 5 ms delay falsely signalled packet readiness on empty sockets. The Matter
receiver acquires the shared RX buffer after this call, then blocks in recv_from;
a ready mDNS socket cannot get that buffer. The full-stack regression injects
one standard wire-format _matterc._udp.local PTR question on mDNS while the
Matter socket remains idle. Before the fix it fails with zero consumed packets;
after the fix it consumes the query and emits a Basic Commissioning response.
Earlier probes checked outgoing announcements/code consistency, not inbound
queries, and therefore missed the starvation. This is a reproduced adapter bug;
it does not prove the unique cause of every hardware allocation failure.

0.1.5 probes recv(MSG_PEEK|MSG_DONTWAIT) with one stack byte, preserving the
queued datagram until actual receive. Empty/transient cases wait 5 ms; readable
(including zero-length) returns success and fatal errors fail. No false readiness,
extra heap-backed poll/select object, SDK private-layout access or protocol-capacity
reduction. Native datagram socketpair tests verify empty/ready/repeated peek,
payload retention, zero-length and closed sockets; the same real-Rust host test
checks inbound DNS through the full stack and the existing expiry/power policies.
Actual radio/IPv6 multicast delivery, Alexa pairing, sustained RAM and OTA still
need an owner hardware test. No real network/Echo was used in the host regression.

Owner 0.1.3 evidence: no abort/backtrace/panic appears in this excerpt; one
accepted config save has its intended restart, and a later POWERON_RESET does
not prove a software crash. The allocation-failure hook reaches >24,000 events.
Reports include 1528/2308 bytes with INTERNAL|DEFAULT (0x1800) and 442 bytes
with INTERNAL|DMA|8BIT (0x80c); heap minima reach 180 bytes and BLE_NO_MEMORY
recurs. Failures start with the window closed. Later the window opens but the
fabric remains unpaired; the owner says Alexa cannot find the device. This
establishes pressure, not the precise allocator caller or a unique cause of
failed discovery. No raw logs or device identities are retained.

0.1.4 bounds Wi-Fi's public init config after Arduino 3.3.12's hard-coded 32/32
dynamic defaults. This classic-board profile uses TX<=4 and RX<=8, raising the
RX target only when required by existing static RX or Block Ack minima. Already
stricter limits are retained. All unrelated fields, storage/callback/magic data,
static buffer counts and aggregation remain unchanged. A linker wrapper forwards
the copied config once to the real SDK init; no SDK fork/private casts. Startup
logs show actual counts, and final ELF validation requires the wrapper.
https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/WiFiGeneric.cpp
https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/api-guides/wifi.html
This bounds frame-buffer growth at the cost of peak throughput; actual RAM
headroom and discovery are not established by the build.

Temporary UDP send pressure now yields in 10 ms intervals, with a 1.5 s retry
window; permanent errors, timeout and partial writes return failure. No false
success or partial-datagram resend. Temporary receive errors remain pending.
The real Rust host driver test covers eventual success after two shortages,
hard/short failure, timeout and cancellation with socket reuse. C++ tests cover
public config copy/limits/minima and errno classification. Serial counters show
UDP TX/RX/temporary failures/hard send errors/timeouts, mDNS socket traffic and
reported IPv6 address count, without payloads/addresses/keys. Socket acceptance
is not proof of Echo receipt or successful mDNS parsing. Physical Alexa pairing,
network isolation/IPv6 routing, sustained heap margin and OTA remain open.

Owner 0.1.2 evidence now confirms displayed dashboard, discovery of all four
stations, assignment and successful save. The subsequent configuration restart
and lost session are intentional; retained stations then read SLEEP. Other
POWERON_RESET entries do not establish software crashes. The one explicit panic
has the matching published ELF fingerprint and decodes to newlib
lock_init_generic -> UART write -> vprintf -> Wi-Fi/coexistence/ppTask. The
mutex creation returned null. Heap minima reach 92 bytes and BLE_NO_MEMORY is
shown; later 15-second samples are around 14 KiB with an 11 KiB minimum before
the panic. They do not identify its transient allocation or establish a leak.
No open commissioning window, paired fabric or power command is shown.

0.1.3 warms the shared UART/VFS write lock through public console write before
Wi-Fi starts, while RAM is available. Keep the SDK formatter, log levels and
messages unchanged, including float/long-long formatting.
An allocation-failure hook logs only numeric size/caps/count through ROM, once
per second at most, without reading the SDK function-name string. The periodic
counter includes throttled events. Its non-inlined callback and format/counters
are verified in IRAM/DRAM in the final ELF, including skip-compile packaging;
GCC LTO otherwise lost the inline callback's IRAM section in a local probe.
Native tests cover console warmup/failure and hook registration, an invalid ignored string
pointer, throttling and millis wrap. Hook disassembly calls ROM/IRAM helpers.
Underlying allocation peaks, sustained RAM headroom, actual OTA and physical
stability remain unverified; this is not a general OOM or Alexa pairing fix.

Owner 0.1.1 evidence confirms Matter/BLE ready without the prior cache panic,
but the dashboard is unavailable after WLAN setup. Data heap drops to about
2412 bytes (largest block 2036), later as low as 1652 (largest 916). A later
POWERON_RESET is shown without a panic; it does not establish an OOM-triggered
reboot. The foreground stack watermark is >=18612 of 24576 bytes: observed
startup/idle use is <=5964 bytes. Pairing/OTA peaks were not measured.

0.1.2 releases 16384 bytes of fixed reservation: the ESP32 future arena is
16384 instead of 24576, and the foreground task is 16384 instead of 24576.
The exact operational future (about 12 KiB on Xtensa) is checked at compile time
against its arena, including alignment and 1 KiB reserve. 64-bit host builds use
24576 for their larger pointer-dependent frame. The operational driver still
uses one arena allocation; no SDK source/private layout casts are introduced.
Arena used/capacity are logged. BLE remains 8192 bytes and every protocol/station
capacity stays unchanged. Hardware HTTP delivery, actual OTA, pairing stack peaks
and Alexa interoperability remain unverified for this correction.

Owner hardware evidence for 0.1.0 is negative: repeated cache-error resets occur
before Arduino setup or HTTP. The exact published ELF decodes the failing PC to
Rust's weak flash-resident memmove, called by ESP-IDF read_id_core during flash
initialization. The 0.1.1 link explicitly resolves SDK libc before Rust, retaining its cache/PSRAM
workaround rather than replacing it with fixed ROM addresses. Packaging checks
memcpy, memmove, memset, memcmp and strlen in the final ELF for strong ROM/IRAM
definitions. Regression tests reject the actual failing address and missing,
duplicate, weak and flash definitions. Owner 0.1.1 now confirms physical startup; HTTP and Alexa pairing remain
unverified. Recover 0.1.0 over four-part USB with erasing unchecked; its HTTP
OTA service never starts.

The owner explicitly replaced the Tasmota experiment. Its active firmware,
installer, runtime, packaging and tests have been removed. Historical Tasmota
results below describe the retired implementation and are not evidence for the
new Rust stack. rs-matter is pinned to 43f61585a439f1a1e90924b696a6280850ef0145;
its no_std library links with the existing production C++ dashboard/API/OTA and
single NimBLE worker. The final local build is 1,836,704/1,966,080 app bytes, with 66,924 static RAM
bytes. Runtime Rust objects are allocated separately; these linker figures do not
establish operational headroom. The Arduino foreground task reserves 16 KiB and
the BLE worker retains 8 KiB. Runtime diagnostics report heap and stack watermark.

Four real-Rust host tests pass, including actual stack startup with synthetic
network/NVS shims, correct verifier/manual-code agreement, renewal, expiry, 0/16
and sparse endpoint models, invalid configuration rejection and no startup/window
power commands. Explicit timed commands expire once and yield to superseding
command generations. These tests do not exercise a physical network or flash.
All sixteen native C++ programs, 31 Python checks and nine simulated browser/USB
programs pass locally. The stable firmware also builds. Packaging checks exact
partition equality, classic chip ID, slot bounds, identical OTA payload and
rebuild sources; Cargo dependency licenses are included with ZIP-safe timestamps. No device is
attached and no successful Alexa pairing, runtime heap margin or actual OTA
has been established for rs-matter.

## Tasmota pairing-control UI correction 0.1.6-tasmota

Owner 0.1.5 evidence: window opens and advertises with IPv6 present and no
transport/mDNS errors. At rx=35, complete=0 and fabrics=0, the IM trace reports
0x30/command 6 -> UnsupportedCommand (0x81). Certificate-chain requests and
AttestationRequest continue afterwards. The excerpt ends there, without later
status, CSR/AddNOC or completion. It cannot establish the optional probe as the
cause of GS014. TC is not advertised. The reference CHIP server rejects this
command when TC is disabled:
https://github.com/project-chip/connectedhomeip/blob/master/src/app/clusters/general-commissioning-server/GeneralCommissioningCluster.cpp
No implicit terms acceptance or protocol-capability change is introduced.

The UI flicker is reproduced by foreground busy handling on each two-second
private status poll. Tasmota's generated UI now distinguishes background polling,
queues explicit manual actions and preserves unchanged text nodes. A held real
page fetch plus MutationObserver checks both button disabled attributes and
name/status/code children. Manual Show during a held fetch results in two reads
and no extra credential mutation; manual renewal during a held fetch executes and
returns its fresh code. Closed windows show explicit guidance. Expiry/error/logout
privacy and action-retry behavior remain covered. SDK/default embedded UI stays
unchanged. Browser uses mocked HTTP/USB; no hardware flash or Alexa pairing is
performed locally. GS014 remains unresolved pending a longer device trace.

Local 0.1.6 build: app 1,710,816/1,966,080 bytes, static RAM 73,348 bytes.
All 25 Python checks and nine browser programs pass, including the real pinned
Berry runtime test and generated gzip dashboard checks.

## Tasmota Interaction Model diagnostics 0.1.5-tasmota

Owner 0.1.4 evidence confirms an open advertised window, rx=79, secure opcode 0x32,
IM opcode 1, local complete=1 and fabrics=1 with IPv6 and zero counted send,
unacked, receive or mDNS errors. Alexa still reports GS014. A later attempt reaches
rx=120/PASE opcode 0x24 and fabrics=0 without another completion, then a power-on
reset. The owner reports removing both stations and the Matter bridge in Alexa
before testing. Exact deletion-to-log timing is unknown. Do not infer automatic
fabric rollback, new crypto failure or successful Alexa registration from this.

The prior diagnostic counters omitted attribute/command/status failures. Added
hooks cover solo and pull attribute evaluation plus solo/batched invoke responses and write-attribute statuses,
preserving the original request IDs before response mutation. The recorder accepts
integer metadata only, uses eight static records, counts discarded history and
preserves first/last errors. Known General Commissioning and Operational Credentials
response enums are explicitly allowlisted; certificate/CSR/nonce/key payloads are
never interpreted as status. Native tests cover saturation, capacity/ordering,
invalid inputs, 32-bit paths and client-error retention after a successful status.
Real pinned host Berry tests exercise attribute data/status serialization and a
command whose response ID differs from its request, including rejected commands.
This uses synthetic transport/data and does not reproduce Alexa GS014 locally.

Serial metadata remains on the 15-second cadence. R/W/C:endpoint/cluster/id=status
contains no values or identities. 000 is a handler-level result, not proof of client
receipt; 100=ignored/deferred, 101=attempt without recorded result. For 201..2ff,
subtract 0x200 to get the whitelisted commissioning response error enum. Other
<=ff codes are IM status. Unsupported optional attributes/commands can be normal.
Read totals count evaluations, including repeated pull/subscription work. Actual
hardware pairing and the precise failure cause remain unresolved.

Local build: 1,710,672/1,966,080 application bytes; static RAM 73,348 bytes.
The updated native diagnostics suite, all 25 Python checks and nine browser/USB
programs pass. Real host tests include read-status serialization, a mutated invoke
response ID, numeric commissioning error extraction, certificate-field exclusion
and write-status metadata with a synthetic sensitive value excluded from records.
These are software checks; actual late-stage Alexa behavior remains unverified.

## Tasmota manual pairing-code renewal 0.1.4-tasmota

Owner 0.1.3 evidence now shows local IPv6 and zero mDNS errors after startup and
observed reconnect recovery. During the new search, window=closed/advertised=no,
rx=0 and one retained fabric persist. Alexa requests a fresh code. The beginning
of the attachment includes older diagnostics before the firmware restart; do not
attribute that older completed exchange or absent IPv6 to the new firmware.
This sample cannot establish a fresh pairing failure or GS014 resolution.

Manual renewal uses pinned valid random PIN generation, code formatting/checksum
and SPAKE2+ verifier construction. The verifier captures the fresh credentials;
root fields are restored afterward, including on errors. Per-window state stays
in RAM; no credential/fabric/identity NVS writes or power calls are introduced.
The local action uses Tasmota's Basic window implementation, not a fabricated
administrator fabric or controller-created Enhanced Window. The private API hides
codes when pending/closed/failed or when the current salt belongs to another
administrator's enhanced window. Empty JSON object, personal session and Origin
checks remain required. Readiness no longer requires a code for a closed window.

Host Berry tests cover different successive codes, bootstrap restoration, captured
verifier PIN, 900-second duration, closure, generation failure and foreign-window
privacy with synthetic crypto/checksum bindings. The production bindings remain
unchanged; host tests do not validate real cryptography or device pairing. Browser
coverage uses generated Tasmota gzip HTML and mocked API, including stale-code
clearing, renewal, expiry, copying, EN/DE and logout. Default SDK gzip equality is
checked separately. Physical Alexa fresh-code pairing remains unverified.

Local 0.1.4 build: 1,708,272/1,966,080 app bytes and 73,124 static RAM bytes.
All 25 Python checks and nine browser/USB programs pass. Additional generated-UI
coverage verifies pending-code hiding, failed renewal/retry, private-fetch failure
clearing/recovery and expiry. Host lifecycle checks include a failing window-close
operation: bootstrap values restore and no incorrect code is exposed. These are
software results; actual new-code Alexa commissioning remains unverified.

## Tasmota network lifecycle 0.1.3-tasmota

Owner 0.1.2 evidence: rx=0/35/79, locally handled complete=1, fabrics=1,
last secure opcode 0x32, last IM opcode 1, final general/credentials masks
0x55/0xc55. send_fail/unacked/rx_error remain zero. mdns_error=1 occurs before
incoming traffic and ipv6_local=no throughout. The owner confirms continued
Alexa GS014. This establishes protocol progress, not successful Alexa setup or
the exact cause of the app error. Free byte heap in the excerpt stays above 47 KiB;
it is a short sample, not a long-run memory guarantee.

Pinned WiFiManager autoConnect calls WIFI_OFF when a hostname is set; Arduino
NetworkInterface destroys its event group and clears initial bits, losing the
earlier IPv6 request. The base Matter Device constructor starts immediately on an
available network, while our lh_begin called start again. Native mDNS add registers
an existing delegated host again instead of updating its addresses. Version 0.1.3
reapplies IPv6 after connection, bounds missing-address retries, guards duplicate
start and refreshes existing A/AAAA records using the SDK update API. No busy
wait, host/service removal, credential reset or power command is introduced.

Native coverage exercises connection loss/reconnect, missing-address retry bounds
and millis wrap. Real pinned Berry exercises duplicate start, stopped-transport
restart and delegated address refresh after IPv6 appears with synthetic transport,
clock and mDNS. Host tests do not prove actual IPv6 allocation, mDNS delivery or
Alexa pairing. Hardware retesting remains required.

Local software results: 1,707,392/1,966,080 app bytes; static RAM 73,124 bytes.
The updated native policy suite, all 25 Python tests and all nine browser/USB
programs pass. Browser/USB use mocks; no device flash or pairing was performed.
These sizes are linker figures, not measured device headroom.

## Tasmota pairing diagnostics 0.1.2-tasmota

Owner 0.1.1 hardware evidence reaches Matter/BLE ready, scans four devices,
saves configuration with the planned restart, reads all four, and confirms one ON
command with raw 11. Ready free byte heap is 68,600/69,340 bytes. Intermittent BLE
574 cycles recover in the excerpt. No successful Alexa commissioning is shown;
the owner reports GS014 after the app's connection screen with a fifth-generation
2022 Echo. Neither the exact failed protocol step nor the post-error paired state
is captured. This supersedes the earlier no-start result, not the pairing limitation.

New fixed numeric hooks report receive/dispatch, root commissioning commands,
send/unacked errors, receive exceptions, mDNS exceptions and locally handled
CommissioningComplete. They never forward payloads, peer addresses, fabric IDs,
secrets or exception strings. Serial summaries are limited to every 15 seconds,
with saturating boot-lifetime counters, command masks and a 0xffff unseen-opcode
sentinel. IPv6 presence, commissioning-window and announcement state are booleans;
fabric count contains no IDs. Native coverage checks masks, ACK filtering,
invalid events/shift bounds and saturation. Real pinned host Berry exercises the
root command and receive hooks with synthetic traffic, checking integer-only
metadata. Tests are software evidence, not a GS014 reproduction or hardware fix.

The local build uses 1,706,384/1,966,080 app bytes and 73,060 static RAM bytes.
The updated Tasmota native suite and all 25 Python tests pass. These linker
figures do not establish runtime margins. All nine browser/USB programs pass
with mock HTTP/serial and real embedded gzip rendering; no device was flashed.

## Tasmota native mDNS correction 0.1.1-tasmota

Owner hardware evidence for 0.1.0 shows import_error in lh_begin, no ready worker
release, free_8bit 71,984/73,892 and complete HTML responses. This supports HTTP
recovery and the reported byte-heap figures at failed startup; it does not establish
operational memory, discovery or pairing. The setup panel's bridge name is present
while its setup code is intentionally withheld until readiness.

The retained original linked ELF contains json, crypto, introspect, path, math,
string and matter modules, but no be_native_module_mdns. A new ELF dependency
check reproduces that failure on the original build. Dedicated USE_BERRY_MDNS
now enables the registry entry, native C module and C++ bindings without enabling
generic USE_DISCOVERY. Required imports are probed before model construction;
failures log only fixed dependency names. Browser coverage checks MATTER_START_FAILED
keeps scan/power/code unavailable but firmware recovery enabled in EN/DE.
No saved identity, pairing credential, fabric, BLE power rule or partition is reset.
Local 0.1.1 build: 1,704,880/1,966,080 application bytes (261,200 bytes reserve),
73,004 static RAM bytes. The linked dependency check, all 25 Python tests and
all nine browser/USB programs pass. The fourteen native suites from the full
prototype remain unchanged. Linker RAM is not measured runtime headroom.
Hardware readiness/pairing/RF remain pending the owner's retest; no ESP32 or
Echo is attached locally.

## Experimental Tasmota Matter prototype 0.1.0-tasmota

This isolated variant links a pinned Tasmota/Berry Matter engine to production
Lighthouse services and the single sequential NimBLE worker. The light adapter is
solidified in flash. It uses only measured OnOff/Reachable updates and queues
explicit On/Off/Toggle, with fresh-only toggle and the existing coalescing/retry/
confirmation policy. Root/aggregator/first-light and persistent registry identities
are retained. Names remain saved unchanged; Matter labels retain a UTF-8-safe
32-byte prefix. Source archives include the complete modified Tasmota sources and
licenses, excluding compiler caches and private attachments.

Native policies cover all sixteen endpoint bindings, duplicate active commands,
measured-state/timestamp preservation, stale toggle, unavailable worker and
maintenance rejection, reassignment mismatch and full-partition erased checks
(including read failures, boundaries and nonempty final bytes). A real host Berry
VM executes the actual pinned plugin classes and the production Lighthouse class:
zero/one/sixteen-light composition, retained endpoint 1, aggregator 2 PartsList,
measured initial states, command dispatch without optimistic changes, Reachable,
identity attributes, failed/unsupported commands and generic-virtual write rejection.
Transport/clock/events are host fakes; this is not an ESP32/Alexa pairing test.

The Tasmota web/console/MQTT/raw-OTA/reset/erase/partition-resize paths are disabled.
Lighthouse alone owns Wi-Fi, protected HTTP/API/USB and inactive-slot validated OTA.
HTTP sends/upload chunks pump the Berry fast loop outside VM calls; worker threads
never invoke the VM. Planned restarts checkpoint fabric state and stop Matter.
Filesystem mounting never formats nonempty data; the full fixed partition must
read as erased before initialization. Tasmota fabric/settings files are separate
from retained SDK fabrics; switching engines requires a new pairing.

The initial complete build used 73,004 static RAM bytes versus 123,284 for the SDK
prototype, and fit both unchanged app slots. These are linker figures, not measured
runtime headroom. All three firmware builds, fourteen native suites, twenty-four Python checks
and nine browser/USB programs pass locally. Final Tasmota app size is
1,700,960/1,966,080 bytes (265,120 flash bytes reserve); static RAM is 73,004 bytes.
The host test also checks the actual On/Off Light device-type serialization.
An extracted-source check builds the host Berry compiler and renders the UI
without a Git checkout; executable permissions are restored for ZIP extractors.
The declaration-only NimBLE uint8_t compatibility patch removes a GCC LTO type
mismatch without changing BLE behavior. No runtime-memory claim follows.
Actual first setup, BLE/Matter coexistence, four/sixteen-station hardware operation,
Alexa pairing/external state, session/identity persistence, OTA/power loss and
return across framework versions remain owner device checks.

## Experimental cleanup correction 0.3.9-matter

The private owner 0.3.8 attachment contains repeated software-reset aborts. The
matching retained CI ELF has the reported SHA prefix. It decodes the abort as
LazyRegisteredServerCluster<ESPMatterDescriptorCluster>::Destroy, Descriptor
shutdown, endpoint::destroy and the application's rollback. Most cycles complete
station 1 with roughly 3.1–3.4 KiB free byte heap, then fail command/cluster
allocation while constructing the next light. One abort occurs without a preceding
station-complete line. No successful HTTP recovery or full startup is shown.
The 0.3.8 recovery claim therefore has negative hardware evidence. No raw log or
private identifiers are retained here.

SDK 1.5 destruction invokes shutdown callbacks without requiring successful init;
LazyRegisteredServerCluster::Destroy asserts when never constructed. The new
production cleanup helper removes never-initialized metadata with public
cluster::destroy before endpoint::destroy. Explicit bounded enable history selects
the normal shutdown path only for initialized endpoints. Creation/resumption also
explicitly disables bare endpoints; SDK enabled state alone does not prove init.
The root and saved identities/fabrics remain intact. Admission checks use a minimum
4096-byte light estimate, raised by completed light construction costs, plus
2048 bytes of byte-heap headroom. This is conservative admission, not a proof that
a future allocation will succeed or that sixteen lights fit this hardware.

Native lifecycle tests reproduce an uninitialized shutdown failure and verify
metadata removal avoids it, initialized shutdown remains intact, deletion failures
are propagated and admission bounds do not underflow. Actual SDK runtime cleanup,
saved-station startup, HTTP recovery, RF and OTA need an owner hardware retest.
Both firmware builds, thirteen native suites, nineteen Python checks and all
browser/USB simulations pass locally. Matter app size is 1,963,600/1,966,080 bytes
(2480 flash bytes reserve); static RAM is 123,284 bytes. No hardware runtime
margin or successful saved-station boot/recovery is established by these checks.

## Experimental saved-station startup recovery 0.3.8-matter

The private owner 0.3.7 log confirms the save reply accepted=true/complete=true,
then the planned restart followed by a boot loop. The retained public release ELF
matches the reported SHA prefix. Most decoded stacks reach lock_init_generic from
UART VFS/newlib printf/ESP logging called during On/Off Light cluster construction
(Scenes or Groups command creation). Another boot explicitly reports endpoint
allocation failure, releases the BLE worker anyway and aborts in throwing
NimBLEDevice::createClient. This identifies allocation-sensitive failure paths,
not malformed saved IDs or proof of a leak. Core-dump write errors follow aborts.
No raw log, hardware identifier or credentials are stored in this report.

The new startup reserves the unchanged worker stack, starts HTTP, initializes
client/scanner-only NimBLE, then waits for Matter completion. USB stays serviced; HTTP is deferred for the
first three seconds of model construction, then serviced if initialization is
still pending, preserving recovery without the usual page/model allocation overlap.
The worker is released only on complete success. Before Matter a 4096-byte recovery
reserve is held and SDK console locks are primed; a minimal allocation hook only
records failures. On the CHIP task, construct all endpoints/Descriptor clusters,
check heap/model health and then enable. The policy checks reserve/free/largest
availability; they do not measure or prove actual working memory margins.
Failure releases the reserve before public SDK destruction of non-root RAM
endpoints, clears active lamps, invalidates root PartsList and reports a startup
error with HTTP/USB/OTA available. Saved station records, registry identities and
fabrics are retained. Radio requests remain rejected. A preflight heap guard also
prevents the shown low-memory client allocation from being attempted. No stacks,
MTU, packet pools, retries, partitions or power commands are changed.

Native tests exercise the production startup order and model batch policy with
construction, activation, allocation/health failures and success for bridge+16
lights. They check reserve release before rollback and no worker/partial-ready
publication on failure. Source ZIP tests reject private attachment paths and check
all local Matter headers/build requirements. A real device is needed to validate
SDK callback/error behavior, RAM recovery, actual four/sixteen-station startup,
BLE coexistence and OTA. Both firmware builds, thirteen native suites, nineteen
Python checks and all browser/USB flows pass locally. The Matter app is
1,963,088/1,966,080 bytes (2992 flash bytes reserve); static RAM is 123,284 bytes.
These figures are not runtime heap/stack margins or a successful device retest.

## Experimental HTTP/configuration memory correction 0.3.7-matter

Owner evidence for 0.3.6 shows discovery completes in five seconds with four results
and no shown scan abort. Subsequent page sends take seconds or fail, with low free
byte-addressable memory and small contiguous blocks. The browser reports an error
when adding stations, but the excerpt contains no handler save-phase result. Wi-Fi
status/reconnect errors also occur; their relationship is not established. No raw
log, address, key or other credential is retained. This does not establish a leak.

Code inspection identifies unchecked partial JSON writes and a save path keeping
request copies, a full JSON arena, proposed configs, restored states, saved JSON,
planned registry, its full arena and a second JSON String live at once. The new
Matter adapter uses protected WebServer hooks for checked <=256-byte socket writes
and moves its protected plain argument into owned in-place JSON parsing. It retries
transient send errors with bounded writer budgets and closes incomplete responses.
The status serializer counts and sends the same immutable snapshot with fixed
Content-Length. Type-erased buffered sinks share one serializer instantiation.

The save path uses bounded stack parsing/configs and frees the request/parser before
separate registry and station serialization. A full-size parser+config workspace
is compile-time bounded to 4096 bytes on ESP32, not a stack high-water measurement.
Only one counted/exact-reserved NVS String is live per phase. Keep transition
checkpoint-before-station ordering, all 16/32 records, public ID fields and hidden
Hue identities. Runtime configs remain paired with old readings until restart.
The response contains new assigned IDs and restarting=true; grace starts after the
response attempt. No power command, reset of settings or partition change occurs.

The host tests compile the production HTTP adapter against WebServer/clock stubs
with real local socketpairs, small send buffers, partial transmission, peer
backpressure, no-progress/global writer budgets and millis wrap. They check moved
body ownership, zero-copy 16 maximum-name station parsing, 32-binding restoration,
exact counting/public replies and serialization failure. Browser tests check one
JSON request in flight and an accepted save without fetching status into restart.
Matter alone builds NimBLE for one simultaneous connection; MTU, pool counts and
all stacks stay unchanged. This does not establish runtime heap headroom. Pinned
build-only Zopfli emits standard gzip, checked against the original UI bytes.
Local verification passes: both firmware builds, twelve native suites, 16 Python
package/layout checks and all browser/USB suites. The Matter app is
1,965,440/1,966,080 bytes (640 flash bytes reserve), static RAM 123,268 bytes.
These linker figures do not establish free heap or stack high-water. Tests include
real socket pressure with delayed draining and writer budgets, one browser JSON
request in flight, 16/32 serialized/restored records and an announced restart that
keeps power disabled without an immediate failing status fetch. Actual hardware
HTTP/NVS/save, startup with saved stations, BLE/Wi-Fi coexistence and OTA remain
unverified.

## Experimental scan result allocation correction 0.3.6-matter

Owner evidence for 0.3.5 contains a successful complete page transmission, active
five-second discovery, then abort/CPU reset at completion. The retained CI ELF
hash matches the reported ELF prefix. Decoding against that exact ELF identifies
`discoverStations -> NimBLEScan::getResults -> vector<NimBLEAdvertisedDevice*>
copy -> operator new -> exception allocation -> terminate`. One generic allocator
frame has a remote-service pointer name due to shared optimized code; the concrete
copy/return frames identify scan results, not GATT service discovery. This is
allocation-failure evidence at scan return, not proof of a leak or a count of
ambient advertisers. Core-dump write errors follow the originating abort. No raw
log, device identifiers, key or pairing credentials are retained.

The pinned 2.5.2 library stores surrounding advertised-device objects by default
and returns its result vector by value. Matter 0.3.6 uses the public legacy GAP
scan API of the same NimBLE host with fixed worker-owned collection instead.
No advertised-object/result-vector/string allocation occurs in the capture path.
Active discovery remains 5000 ms; passive recovery remains 2000 ms. Both retain
97/67 ms interval/window, controller duplicate filtering and single-worker radio
ownership. The GAP callback only parses/copies bounded reports; it never connects,
locks production state, performs serial/network IO or writes power. The worker
waits on atomic completion before using/releasing its context. Failed starts do
not wait, completion codes are logged and failed discovery retains earlier data.

The shared parser handles complete/shortened name fields, malformed lengths,
canonical MAC order, repeated MAC updates, 16-result overflow and unrelated names.
The recovery collector accepts only the assigned MAC and explicit ADV_IND or
ADV_DIRECT_IND connectability, preserving observed type/RSSI without persisting
changes. A host test compiles the actual Matter callback/runner against GAP/RTOS
fakes, injects 4000 unrelated plus 4000 matching reports and asserts no C++
allocation, correct mode/duration,
completion ordering and start/end errors. These fakes do not exercise the actual
controller/host allocator, callback scheduling or RF behavior on ESP32.
Local builds pass for stable 0.3.9 and Matter 0.3.6. The Matter app is
1,962,720/1,966,080 bytes (3360 bytes flash reserve), static RAM 123,716 bytes.
All eleven native programs, 16 Python package/layout checks and all browser/USB
suites pass. ELF symbol inspection confirms the old NimBLEScan::getResults/result
copy path is absent from this Matter build. These checks do not measure runtime
heap or actual RF behavior. Hardware discovery, memory/coexistence, OTA and
multi-station reliability remain unverified.

## Experimental status heap correction 0.3.5-matter

Owner 0.3.4 evidence confirms ready startup, visible OUT_OF_MEMORY/20, a failed
page transfer followed by a complete one, and repeated failure of the 4512-byte
status allocation. Arduino reports an unchanged largest internal block around
9 KiB, but its helpers request MALLOC_CAP_INTERNAL, including word-only IRAM on
classic ESP32. This cannot establish availability for a byte-addressable snapshot.
No raw credentials/log/identifiers are retained. The changing free-heap samples
alone do not establish a leak. The provided resets report POWERON_RESET, not a
recorded application panic or an identified core-dump failure.

0.3.5 owns status entries in individually checked blocks <=256 bytes, allocated
only for actual station records and discovery results. All copying/uptime capture
is under the same lock, with owned strings and no network IO under the lock.
Metadata is <=512 bytes on stack. Maximum capacity stays 16+16 without truncation;
partial allocations are released and rejected before a response begins. The
firmware and host tests use the same capture/serializer. Native tests simulate a
heap refusing blocks above 256 bytes, empty and sparse slot-16 configurations,
all 16 stations/results, a newer source measurement after capture, and failures
at every allocation with complete cleanup and no partial serialized response.

New free_data_heap/largest_data_block fields measure INTERNAL|8BIT; old status
heap fields retain Arduino semantics. Page/status/startup logs label byte-memory
values explicitly. The generated Matter dashboard normalizes non-string browser
error codes to NETWORK_ERROR; the browser test injects AbortError (code 20) and
checks the readable error key. No extended timeout or station limit is introduced.
Local verification passes: both firmware builds, all ten native programs, 16
Python package/layout checks and all browser/USB suites. The Matter app is
1,965,952/1,966,080 bytes (128 flash bytes reserve), static RAM 123,724 bytes.
These are linker figures, not runtime margins. Browser checks cover real localhost
gzip decoding/login and injected AbortError normalization, with API operations
mocked. Repeated native capture/replacement cycles return every block and reject
oversized discovery counts. Real runtime memory, rendering, BLE coexistence,
OTA and multi-station operation remain hardware checks.

## Experimental blank-page follow-up 0.3.4-matter

Owner evidence for 0.3.3 confirms worker reservation/release, Matter endpoint
readiness and HTTP readiness. The owner explicitly reports a completely blank
browser page. A request to misspelled `/api/metter` returns NOT_FOUND; the correct
`/api/matter` remains private and requires an own-password session. No raw log,
credentials or hardware identifiers are retained. The core-dump partition warning
alone does not establish an application crash; shown resets report POWERON_RESET.

The compressed dashboard was approximately 17 KiB and passed to one socket write.
Reported largest free blocks are approximately 9 KiB. The pinned NetworkClient
passes the whole remaining length to nonblocking `send` and stops on errors other
than EAGAIN; TCP buffer pressure is a possible explanation, not a reproduced cause.
0.3.4 sends at most 512 bytes per write directly from mapped flash, handles partial
writes, closes incomplete responses and logs actual body byte counts/time/heap.
It does not allocate a full page or change partitions. Complete byte counts prove
socket acceptance only, not browser delivery or rendering. A native binary-payload
test covers a body larger than the reported largest block, embedded zeros, partial
writes, a short final chunk, disconnect and empty input.

The zero default hostname suffix is separately addressed using ESP-IDF STA MAC
retrieval independent of Wi-Fi netif readiness, retaining the original last three
bytes and custom-name override. No NVS erase or credential rotation is performed.
Local verification: stable 0.3.9 and Matter 0.3.4 builds pass. The Matter app is
1,965,696/1,966,080 bytes (384 bytes flash reserve); static RAM is 123,724 bytes.
This is not runtime memory headroom. All ten native programs, 16 Python tests and
the complete browser/USB suite pass. The browser test additionally serves the
actual generated gzip bytes over real localhost HTTP in 512-byte writes, checks
browser decoding against the original HTML and waits for the visible login form
without JavaScript errors. Other API interactions remain mocked. No actual ESP32
web rendering, memory/coexistence, OTA or multi-station success is claimed.

## Experimental worker startup/recovery 0.3.3-matter

The owner's 0.3.2 boot log reaches Matter endpoints ready, BLE initialization
success and then "Could not start BLE task". HTTP registration follows that
infinite-wait branch, explaining the inaccessible web interface. The log does
not contain allocation/free/largest-block diagnostics; insufficient contiguous
memory and total exhaustion cannot be distinguished from this evidence alone.
The credential-bearing raw log and identifiers are deliberately not retained.
The separate missing-core-dump-partition startup diagnostic is not the recorded
stop point; the partition layout is unchanged.

The corrected startup reserves the same 8192-byte worker stack first. An atomic
gate prevents all worker radio operations until BLE initialization succeeds.
Temporary boot-configuration text is freed before reservation. HTTP starts before
Matter/BLE initialization; worker allocation, BLE-init and coexistence-policy
failures return to the serviced recovery interface instead of an infinite wait.
Matter commands/pairing readiness also check worker availability. Radio APIs reject
unavailable work with 503; the UI disables radio actions while keeping OTA usable.
Reservation logs add only status/stack/free/largest values, without credentials.

The native test exercises the production startup orchestrator: reservation before
Matter, HTTP before initialization, no worker reads before release, recovery on
worker/driver failure, local radio service despite a Matter-only failure, and no
automatic writes. The browser test checks unavailable-radio controls and continued
firmware-upload access. These simulate failures, not actual heap allocation on
ESP32. The local app uses 1,965,328 of 1,966,080 slot bytes (752 bytes reserve)
with 123,724 bytes of linker static RAM; these are not runtime heap margins.
Early reservation is a correction to allocation order, not proof that the
full runtime always has sufficient memory. Actual memory/coexistence/OTA remain
hardware checks. The stalled 0.3.2 needs settings-preserving USB recovery.
All nine native tests, 16 Python checks and the complete mock browser/USB suite
pass locally. These are software checks only.

## Experimental Matter startup correction 0.3.2-matter

The owner reports only the bridge name after clicking Show setup code, without a
status message. Their private-info response reports version 0.3.1-matter,
ready=false, paired=false, error=MATTER_BRIDGE_FAILED and pairing_code=null.
It contains no usable pairing code; UI rendering alone cannot create one.

The pinned SDK's endpoint resume implementation rejects IDs at or above its
minimum-unused counter. Local binary disassembly confirms the bound and SDK
metadata namespace/key. The previous adapter resumed bridge 2 before Matter.begin
restored that counter. The corresponding upstream implementation is documented in
[Espressif's data model source](https://github.com/espressif/esp-matter/blob/main/components/esp_matter/data_model/esp_matter_data_model.cpp).
The corrected lifecycle starts the root stack, then schedules endpoint creation
on the CHIP task, reading its counter without modifying SDK storage directly.
New IDs use create; existing IDs use resume. Retired gaps use empty, destroyable
placeholders that are removed before registration. Finished endpoints are enabled
and root/aggregator PartsList reports are invalidated. Commands remain gated by
readiness; config saves are blocked during initialization. No power write/reset
or station-identity migration is introduced.

Native tests reproduce pre-start resume rejection and exercise fresh/restored
counters, legacy IDs 1/2, gaps, all 16 lights, duplicate rejection, final valid
ID/exhaustion, counter mismatch and allocation failures. These tests model the
verified SDK counter semantics; they are not a hardware Matter stack test.

A separate browser reproduction removes the old button's nested code element:
it displays only the bridge name and throws before rendering status. This is a
renderer weakness, not established as the owner's browser cause. The new button
owns its text directly; errors render first. The browser test uses the owner's
failure state, checks the translated error/no code, then a successful code,
replacement button contents, copy from response state, language and logout.

The local application is 1,963,840 bytes in the unchanged 1,966,080-byte slot,
with 123,724 bytes of linker static RAM. The 2,240-byte flash reserve is not a
runtime heap margin. All eight native test programs pass locally.

Actual startup/pairing, runtime memory and BLE coexistence on the ESP32 remain
unverified for this correction. Do not erase NVS to work around this error.

## Experimental Matter memory correction 0.3.1-matter

Owner hardware evidence for full 0.3.0: the HTML page loads, but the interface
reports "The bridge has too little free memory. Try again." The Matter section
is absent. Bluetooth discovery produces no visible results/logs. No startup heap
values or complete trace are available; these observations do not independently
establish that the radio never scanned. The old discovery path had no success
logs, and the UI needs the failed status response to display results.

Code inspection finds a fixed 32768-byte status JSON document plus a second full
serialized String. OUT_OF_MEMORY is returned when that document cannot populate.
The Matter panel was gated by the same failing status response. Version 0.3.1
removes these allocations and uses a single coherent snapshot below 6144 bytes,
bounded per-item stack JSON and 256-byte HTTP chunks. A personal-session account
flag makes the panel independent of status. The code still requires the personal
password; neither public status nor the account flag contains a pairing code.

Native tests exercise the exact production serializer with all 16 stations and
16 results, escaped/UTF-8 names, long errors, fixed IDs, true measurements during
pending commands, read phases, millis wrap/staleness, empty lists and failed
writes. Chunk sizes 1/2/7/16/256/4096 cover boundaries. Capacity tests serialize and
restore 16 maximum-length station names/MACs and all 32 registry checkpoint
bindings using the production capacity constants. The browser reproduces failed
status while retrieving/copying the code and opening the pairing window, then
recovers status and displays discovery results without power calls.

The local corrected app is 1,961,168 bytes in the unchanged 1,966,080-byte
slot (4,912 bytes of flash reserve), with 123,732 bytes of linker static RAM.
All seven native test programs, 16 Python package/layout checks and the complete
mock browser/USB suite pass locally. The new browser regression keeps the Matter
panel/code usable while status returns OUT_OF_MEMORY, then verifies status and
discovery recovery without power calls.
These figures do not measure runtime free heap. Startup logs and status now add
free/largest/minimum heap values for the next owner test.

Actual low-memory behavior, discovery, Matter and uploads on the owner's ESP32
remain a required retest. Software tests do not validate physical runtime memory
headroom. Partitions, NVS and station/Matter identities remain unchanged.

## Experimental full Matter 0.3.0-matter

The owner requested dashboard, API, OTA and web retrieval of the setup code now,
without waiting for the multi-station test. The full build includes production
`src/main.cpp`: authentication, USB recovery, one BLE worker, measurements,
configuration parser and the streamed OTA receiver are shared with stable.
The Matter adapter changes protocol/identity reporting, not physical power rules.
Station saves checkpoint the identity registry before committing configuration,
pause new BLE work and restart after the response. Existing fabrics/endpoints
are retained; no boot/reconnect power write is introduced.

The private setup-code route requires the personal-password session. Its pairing
window mutation checks JSON and Origin, and schedules only a Basic Commissioning
Window (DNS-SD, 15 minutes) on the CHIP task. No API-key/bootstrap access, fabric
reset or pairing code in general status. The generated gzip dashboard is exercised
with mock HTTP for explicit retrieval, window opening, copying on local HTTP,
language preservation, hidden Hue controls, no power calls and code erasure on
logout. These browser checks do not prove device commissioning or OTA operation.

Size optimization uses gzip, C++ `-Oz -flto`, C `-Oz` and `WM_NODEBUG`. The initial
full adapter exceeded the static DRAM region; registry/lamp objects now use
checked heap allocations. Runtime heap/task stacks, SDK/framework NVS migration,
actual OTA/power-loss handling, Wi-Fi/BLE coexistence and multiple lamps all need
hardware tests. Prior owner success applies to 0.1.1's single station only.
The local full app is 1,955,440 bytes in the unchanged 1,966,080-byte slot;
linker static RAM is 123,732 bytes. Only 10,640 flash bytes remain, so every later
change must repeat the size check. These figures do not establish runtime heap
headroom. Stable still builds as 0.3.9. Native request/identity/Hue/OTA checks,
16 Python checks and the complete mock-browser/USB suite pass locally.

## Experimental Matter 0.2.0-matter — multiple stations and agreed naming

The owner explicitly confirms that the 0.1.1 single-station version switches
ON/OFF, has the correct lamp type and displays the correct status. Its screenshot
shows the parent Matter bridge and a separate lamp with OFF. This completes the
agreed first test at the reported level, without proving external-controller
updates, detailed confirmation timing or long-run reliability. No screenshot,
setup code or raw credential-bearing log is committed.

The next experiment exposes every configured station with a nonempty MAC as a
separate On/Off Light, up to 16. The parent is named **Lighthouse-Bridge Matter
Test**, with **Lighthouse-Bridge** agreed for after the test phase. Saved station
names and local IDs remain unchanged. The prior first child remains endpoint 1;
aggregator 2 stays reserved. A private, atomically stored registry binds full
stable identity + MAC to monotonically allocated Matter endpoint IDs, preventing
slot reuse or reordering from controlling a different physical device.

Native registry tests exercise initial migration, retained identities after
reordering, replacement MACs, interrupted-save checkpoints, returning to a retired
MAC, all 16 stations/32 transition bindings, malformed registries and exhaustion.
The build generates the exact production BLE-worker section from `src/main.cpp`;
there is no independent retry/confirmation loop. USB frames preserve old fields
and add the bridge name and a validated list of measured station IDs/names/states.
The browser test covers separate rows and measured updates without power writes.

The full local 0.2.0 binary is 1,826,992 bytes with 121,764 bytes of linker static
RAM. It fits the unchanged 1,966,080-byte slot; these are not runtime heap margins.
Multiple endpoints, retained fabric migration, Alexa's refresh of the name/child
list, four-station switching and 16-station runtime operation need owner hardware
tests. Prior one-station success must not be attributed to this changed adapter.
The native identity/core tests, all 15 Python checks, actual USB parser/watcher
checks and the complete simulated browser suite pass locally for this test build.

The next dashboard/API/Wi-Fi-update phase has not shipped. A separate early size
probe combined the existing services, an SDK Matter light and a gzip UI (14,412
bytes). It used 1,990,075 flash bytes; compiling out WiFiManager's already-disabled
debug output reduced this to 1,970,027, still 3,947 over the slot before the final
multi-station adapter. Those temporary compile probes are not firmware releases
and do not validate full integration. More optimization is required; the owner
has explicitly retained both existing app slots.

## Experimental Matter 0.1.1-matter — light device type

The owner reports that 0.1.0-matter works fundamentally on the physical ESP32,
but Alexa identifies it as an outlet. That matches its deliberately advertised
On/Off Plug-in Unit endpoint. This is positive owner evidence for initial basic
operation, not a controlled timing/confirmation, long-run or external-status test.
No raw serial data, pairing code or hardware identifier accompanies this report.

Version 0.1.1-matter uses the pinned SDK's `MatterOnOffLight` wrapper, which creates
the On/Off Light device type. The existing Matter endpoint/unique IDs, bridged
reachability, per-board commissioning credentials and both app slots are retained.
The existing command handler and measurement-only OnOff guard also remain.
The subsequent owner report above confirms the new light classification and
basic switching/status; the old outlet result alone did not validate this update.
Removing/re-pairing only the prototype entry while connected may be necessary;
it does not require erasing stable Wi-Fi/station configuration.

The local 0.1.1 binary is 1,814,992 bytes and passes the existing slot/partition
validator. Inspection of the linked ELF finds `MatterOnOffLight::begin` and the
SDK's `on_off_light::create`, with the former outlet wrapper absent. All 15
Python checks, USB protocol checks, and the experimental/stable WebFlasher
browser simulations pass. This validates the build and software flows, not
Alexa's eventual classification of an already paired entry.

## Experimental Matter prototype 0.1.0-matter

The isolated classic-ESP32 build uses Arduino ESP32 3.3.12, NimBLE-Arduino 2.5.2,
ArduinoJson 6.21.5 and the existing partition CSV. The owner explicitly declined
larger/single app slots. Initial minimal Matter+Arduino-BLE and raw Bluedroid
probes used 2,270,507 and 2,237,791 bytes respectively, exceeding the 1,966,080-byte
slot. A Matter+NimBLE probe used 1,778,099 bytes and passed that build limit.
The full local single-station prototype binary is 1,814,992 bytes, leaving
151,088 bytes in the same slot. Its linker reports 115,756 bytes of static RAM.
These figures are not a runtime free-heap or coexistence test. Native request,
Hue, config and OTA tests, all 15 Python checks and the full browser/USB suite
pass locally, including the experimental page's simulated installer handoff.

The prototype reuses the shared configuration parser, measurement/queue logic,
bounded read attempts, HCI 0x3e passive recovery and read-only confirmation. It
reads saved assignments without rewriting the stable record and chooses the
lowest assigned local ID. A Matter command handler queues ON/SLEEP; measured
OnOff and bridged Reachable are updated on the Matter task. An attribute callback
rejects OnOff changes outside the measurement-report path. Neither startup nor
Wi-Fi reconnect invokes the power-command path.

The package validator checks classic ESP32 chip headers, all four segment/sector
bounds, both original app slots, exact partition entries and a prototype product
marker. Pages also requires byte-identical stable/prototype partition tables and
all image hashes. Python tests simulate installing the actual prototype segments
and returning with stable segments while retaining saved NVS, and reject an
oversized app, changed partition table, wrong chip and offset-zero image.
Node checks run the actual USB frame parser/watcher with split frames and reject
malformed data. Browser simulations check the separate manifest, experimental
consent, EN/DE mobile layout, one USB owner, code copying/privacy, pairing-state
updates and disabled installation on an unsafe manifest. These do not flash a
device or communicate with an Echo.

No ESP32/Echo is attached locally. The later owner report above supports basic
operation of this first prototype. It does not establish detailed command-handler
dispatch/timing, runtime memory, sustained BLE/Wi-Fi/Matter coexistence, fabric
storage, external-state synchronization or return across framework versions.
The prototype has no dashboard, HTTP API or Wi-Fi update upload. It remains an
experiment rather than a verified status-sync fix.

## Owner discovery/setup Offline report

The owner supplied a screenshot of Alexa's device-naming screen with all four
individual stations labeled **Offline**, and an accompanying serial excerpt
from 21:58:43 to 22:04:39. The owner recalls a status display in the same setup
flow with 0.2.6. This is owner hardware evidence, not a locally reproduced or
controlled firmware comparison; the exact running version and screenshot time
are not included.

At 21:59:10 every station receives two individual Hue GETs reporting
`profile=standard`, `on=false`, `bri=0`, `reachable=true`, `pending=false`, and
`source=measurement`. Successful BLE reads report raw 0 (SLEEP). At 21:59:56
collection GETs report four configured stations and three fresh measurements;
subsequent individual GETs correctly report only station 1 unreachable after
exhausted read cycles. Stations 2–4 remain reachable. Station 1 recovers a
successful read at 22:00:11 and returns reachable=true on later logged GETs.
No Hue PUT is present in the excerpt, so it supplies no switching evidence.

The sample establishes that discovery performs individual status GETs and the
bridge reports all stations reachable in the first burst. BLE failures can
explain the later station-1 reachability change, but do not establish a cause
for all four Offline labels. Handler diagnostics do not prove how Alexa parsed
or received the complete HTTP response. The compatibility profile is not tested
by this standard-profile sample. Response compatibility, device identity and
the reported difference from 0.2.6 remain under investigation; no fix is claimed.
Only this sanitized summary is retained, not the raw log or screenshot.

The owner's subsequent station-1 compatibility-profile test still displays
"Device is unresponsive" in Alexa. In the supplied 22:10:27–22:13:26 excerpt,
station-1 GET diagnostics at 22:10:39, 22:10:52, 22:11:22–23 and 22:13:24
report `profile=compat`, `on=false`, `bri=0`, `reachable=true`, `pending=false`,
and `source=measurement`. Collection GETs at 22:11:22 report `configured=4`
and `fresh=4`. Station 1 repeatedly reads raw 0 successfully. No Hue PUT is
shown, so this does not demonstrate a rejected or failed power command.
The profile experiment has not resolved the owner's symptom in this test.
The excerpt does not independently establish Alexa capability refresh or
successful client receipt/parsing of the complete HTTP response. Neither BLE
unreachability nor missing GETs explains the station-1 warning in this sample.

## Version 0.3.9 optional per-station LWB010 profile

The owner reports 0.3.8 GET on=false/bri=0/reachable=true while the app still shows
ON, including after fully closing the app and clearing local cache. Thus the
extra read fields did not fix the observed symptom; a local-cache-only diagnosis
is not established. No credential-bearing/raw hardware response is committed.

An optional profile, off by default per station, advertises LWB010/Dimmable light
rather than HASS321/On-off. Numeric/unique IDs, measured data and BLE limits remain.
Only compatibility-profile Hue writes accept binary bri 0..254, with explicit on
precedence and at most one queue operation; ACKs report effective 0/254. Standard
profile stays boolean-on-only. This is a compatibility experiment requiring an
owner retest after re-discovering just one changed Alexa device; it is not a
verified fix and may require restoring that device's groups/routines.

Native tests cover profile metadata/identity, binary brightness bounds and types,
explicit on precedence in either property order, one queue call per mixed body,
standard-profile rejection, persisted/default/omitted/invalid config values.
Browser checks cover the checkbox across language changes, save and reload,
with other stations left standard. Existing debug geometry/auth/ID tests remain.
Firmware/package/NVS checks apply. No Echo or ESP32 is attached to these tests.

## Version 0.3.8 experimental Hue state hints

The owner reports switching via Alexa works, but switching OFF through the web
interface leaves the app showing ON. The later log explicitly contains a fresh
Hue GET for station 1 with on=false/reachable=true/pending=false/source=measurement.
A full 0.3.7 collection response contains all four stations with false/true and
valid On/off light/HASS321 metadata. This rules out absent polling in this sample;
it does not prove a cache-only cause or a defect in the app. Raw responses/logs
contain hardware identifiers and are not committed.

Version 0.3.8 adds read-only binary bri=0/254 and mode=homeautomation to all Hue GET
shapes as a compatibility experiment. Device type/model/identities, PUT semantics,
local API/measurement data and BLE behavior remain unchanged. The hint always
matches Hue on, including the existing bounded accepted-target window; it is not
physical brightness. No app synchronization fix is claimed without hardware testing.

Native serializer checks cover SLEEP/ON/Starting/STANDBY/unknown states, collection
and individual/state-only shapes, external ON-to-SLEEP readbacks, accepted-target
expiry, stale reachability and unchanged measurements/timestamps. Existing schema,
identity and ON/SLEEP ACK checks remain. Brightness-only writes are rejected.
Build/package/NVS checks apply. Owner retest must check the app after a web SLEEP
and a new GET, with bri=0/on=false; no Echo or ESP32 is attached here.

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
