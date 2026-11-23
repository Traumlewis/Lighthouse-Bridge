# Firmware validation

## LH-Bridge 0.1.36: C3 commissioning crash workaround

Build the C3 with the pinned compiler's machine outliner disabled. The
packager checks the final ELF for the absence of outlined functions.
The C3 release limits SDK logs to errors to fit both existing slots.
Typed endpoint, session, heap and transport diagnostics remain available.
After the 0.1.35 crash, an authenticated LAN check found all four stations
reachable, Matter ready, no fabric, and the commissioning window closed.
Pairing through Alexa with the new build requires a hardware check.

## LH-Bridge 0.1.35: dashboard reconnect after saving stations

The owner's C3 on 0.1.34 found and saved four stations. The configuration
reply completed, the bridge restarted intentionally, and all four stations
were read afterward. During the full dashboard reloads, available heap
fell and multiple HTTP transfers ended before sending the full body.
The new flow waits for the old session to expire and returns to sign-in
using the loaded page. On C3 a manual dashboard download waits for active
BLE work and prevents another BLE job from starting during transfer.
Physical verification is pending.

[Actions run 45267392693](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/45267392693)
published 0.1.35. Simulated browsers confirm that saving stations returns
to sign-in without fetching the dashboard again, and that a changed
firmware version still triggers a full reload. All six live manifests,
layouts, firmware images, OTA payloads, checksums and source ZIPs passed.
On the owner's C3 SuperMini, an authenticated OTA to the published 0.1.35
returned `OTA_WRITE_FAILED`. A rate-limited retry after BLE drained and
with a 7668-byte largest free block also failed at the HTTP transport.
The bridge remained on 0.1.34 with all four stations reachable.

The owner subsequently ran 0.1.35 and reported recovery of most functions.
USB logs show ongoing station reads and an explicitly opened Matter
commissioning window. All observed UDP traffic was mDNS, with no logged
PASE session or fabric. During that attempt the largest internal heap block
fell to 2292 bytes and repeated 2312-byte allocations failed. These logs
place the failure before session establishment without establishing why the
controller's Matter traffic failed to arrive. A follow-up LAN check could
not reach the last reported bridge address and received no targeted mDNS
response. Commissioning on C3 remains unverified.

A later owner Alexa attempt on C3 SuperMini 0.1.35 established one PASE
session and armed the fail-safe, then rebooted with a Store access fault
before creating a fabric. The panic ELF digest prefix `0b62ee12d` matches the
published C3 SuperMini debug ELF from run 45267392693. In that image the
`CmdPath` Debug implementation puts the command formatter address in `t1`
and then overwrites it with an outlined tail-call target. The outlined body
saves the overwritten register as the formatter. The resulting indirect
call enters a different outlined function mid-instruction, matching the
register dump and store fault. The earlier allocation failures are separate
evidence of heap pressure.

## LH-Bridge 0.1.34: ESP32-C3 heap headroom

On the owner's C3, 0.1.30 reports repeated runtime allocation failures at
2312 bytes and a largest free block as low as 1460 bytes. The dashboard
truncates on one boot, while another boot completes discovery of four
stations. A separate read-only HTTP login-options request yielded an empty
response. This confirms heap pressure and an HTTP symptom. The allocating
SDK component for the 2312-byte failures is unknown.

The change branches from the restored 0.1.30 baseline. The upstream
NimBLE host reserves both pools at initialization. For C3, use 12 instead
of 24 blocks for MSYS2 and incoming HCI ACL, with the block sizes retained.
The C3 ELF confirms 3840 bytes for MSYS2 and 3600 bytes for the ACL pool,
releasing 7440 bytes of initialization-time heap reservations.
Dashboard, discovery, GATT reads and writes, and Matter commissioning
under real C3 load need owner verification.

[Actions run 45260071299](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/45260071299)
published 0.1.34. All six live manifests, chip and flash headers, USB offsets,
exact partition tables, OTA payloads, checksums and source ZIPs passed.
Simulated browser login, dashboard and USB tests passed.

## LH-Bridge 0.1.30 restored as the baseline

The owner confirmed 0.1.30 worked after disabling the VPN and requested
that main development and the root WebFlasher return to this version.
The source matches commit 663a438 apart from documentation recording later
history. The redeployment reuses the original six verified board images.
Actions run 45255471898 published the original site at the project root.
The live HTML, six manifests, USB segments, OTA packages and ZIP downloads
match the original artifact. The deployment archive excludes the temporary
test page.

## LH-Bridge 0.1.33: setup address and captive DNS

In the owner's C3 and S3 logs, 0.1.32 starts the TCP listener and Android
joins the setup AP. The response count was captured before Android joined.
The browser page and Android Wi-Fi settings prompt remain unavailable.
0.1.33 explicitly offers the captive DNS server via DHCP, binds HTTP to the AP
address and reports it over USB while the portal is active. The physical
result is pending.

[Actions run 45243859636](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/45243859636)
published all six 0.1.33 profiles. Simulated browser and USB checks cover the
setup link while the dashboard URL is null, and its removal when the bridge
reports the home Wi-Fi address. Live checks verified the root WebFlasher,
setup link assets, six manifests, firmware chip and flash headers, offsets,
exact partition equality, OTA payloads, checksums and source ZIPs. The
largest published apps are 1,965,760 bytes in 1,966,080-byte slots.

## LH-Bridge 0.1.32: setup portal HTTP

The owner reports the setup connection reset persists on 0.1.31. This
candidate shares the bounded HTTP writer with the main dashboard and
reports setup listener state and attempted response count via USB. The added
fields are redacted and distinguish TCP listen from response attempts. The
cause of the physical reset remains unverified.

[Actions run 45234468473](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/45234468473)
published all six 0.1.32 profiles. Live checks verified the main WebFlasher,
manifests, chip and flash headers, USB offsets, exact partition tables, app-only
OTA payloads, SHA-256 and source ZIPs. Both new USB diagnostic fields are
present in all six apps. The largest published apps are 1,965,632 bytes in
1,966,080-byte slots.

## LH-Bridge 0.1.31: first-setup portal candidate

The owner observed `ERR_CONNECTION_RESET` on both the setup root and the
Wi-Fi page on a C3 with firmware 0.1.30. The phone joined the protected AP
and obtained an address. The owner reproduced the issue on a freshly set up
S3. The C3 log shows two driver initializations before the portal. The
candidate avoids that transition when there is no saved Wi-Fi configuration
and reports portal/AP readiness over USB. The cause of the reset and the
candidate's effect on physical devices remain unverified.

[Actions run 45226038063](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/45226038063)
published all six 0.1.31 USB/OTA profiles. Live manifest, chip, flash size,
offsets, exact partition equality, OTA payload, checksum and ZIP checks
passed. All apps retain both OTA slots. The new diagnostic fields are
present in all six published apps.

## LH-Bridge 0.1.30: Seeed Studio XIAO profiles

The Seeed XIAO C3 and S3 images preserve the complete partition table and both
1,966,080-byte app slots. The local apps are 1,947,344 bytes for XIAO C3 and
1,870,528 bytes for XIAO S3. Both images contain the expected chip and flash
headers, and the OTA payloads match the USB app images. The Arduino properties
confirm native USB serial on boot and 8 MB flash with OPI PSRAM for XIAO S3.
Native checks cover the XIAO GPIO allowlists and both directions of the 4 MB/8 MB
S3 OTA guard. Physical USB, Wi-Fi, BLE, GPIO, OTA and Matter checks are pending.

[Actions run 45163198385](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/45163198385)
published all six 0.1.30 profiles. The live WebFlasher passed EN/DE board
selection, download and consent checks at 1040, 390 and 320 px. All six live
manifests and chip, flash size, offsets, partition table, OTA, SHA-256 and ZIP
checks passed. Published app sizes for XIAO C3 and S3 are 1,947,120 and
1,870,288 bytes respectively.

## LH-Bridge 0.1.29: unified version label

All four firmware packages fit the existing app slots. The asset checks
confirm numeric manifest and download versions. Dashboard regressions
cover the visible `LH-Bridge 0.1.29` label and reload on firmware changes
without a reload on ordinary session expiry. The USB reader accepts both
the new version format and older Matter version strings.

[Actions run 45140983557](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/45140983557)
published 0.1.29. All four live manifests, chip and partition checks, OTA
payloads, SHA-256 values, and ZIP checks passed. The live WebFlasher showed the
same label for all boards in EN/DE at 1040, 390, and 320 px. The compiled
Matter software version string was verified in all four images.

The owner-approved classic ESP32 OTA from 0.1.28-rs-matter to 0.1.29 succeeded
with one upload. The initial verifier stopped before any upload because it
expected 0.1.27. After checking the actual baseline, the OTA preserved the
account, room, network, GPIO, station assignments, OFF preferences and Matter
pairing. All four stations became reachable. The live dashboard displayed
`LH-Bridge 0.1.29`, and both diagnostic sessions were closed.

## rs-matter 0.1.28: main build and WebFlasher

The active release path builds rs-matter for four boards and validates their
chip headers, complete partition tables, OTA payloads, and per-segment hashes.
All four local builds, 26 Python checks, 12 browser and USB programs, the
native regression programs and eight Rust tests in both debug and release
passed. No physical 0.1.28 installation was performed.

[Actions run 45124743206](https://github.com/Traumlewis/Lighthouse-Bridge/actions/runs/45124743206)
published commit 3c6c098. The live WebFlasher passed English desktop and German
mobile browser checks for all four board options, consent gating and OTA/ZIP
links. Downloaded manifests, board chip headers, exact
partition equality, OTA payloads, SHA-256 and ZIP files passed. Published app
sizes are 1,963,792 bytes for classic ESP32, 1,965,168 for both C3 boards and
1,889,760 for S3. The app slots remain 1,966,080 bytes each.

## rs-matter 0.1.27: Dangerzone restart

EN/DE browser checks at 1280, 390 and 320 px cover a rejected restart and
an accepted restart with an old authenticated response, a dropped connection
and the new login gate. The browser then reloads once at the same URL. The
factory-reset flow still presents the new setup password. All four rs builds,
the SDK Matter build, 34 asset checks and 16 browser/USB programs pass. App
bytes are 1,964,048 classic, 1,965,456 both C3 profiles and 1,890,016 S3 in
the unchanged 1,966,080-byte slots.

Owner-approved classic OTA from 0.1.26 succeeded with one upload of the
locally checked build from commit 0a1cd31. Network, account, room, GPIO,
station assignments, OFF choices and Matter pairing were preserved. The
single authorized Dangerzone restart was verified by uptime. All four
stations became reachable again. The browser verifier failed an assertion;
the served-dashboard replay reproduced the mismatch in the observer's
expected request list, which omitted the normal BLE read refresh on login.
With that check corrected, the replay confirmed automatic reload to sign-in
at the same URL after simulated reconnect. The replay did not send a second
restart to the device.

## rs-matter 0.1.26: login and serial number

EN/DE browser checks cover the initial USB sign-in hint, personal-password
login with and without a username, and a full printable serial in Device
information. A host check of the BLE reader and JSON output covered a
NUL-terminated printable serial, a four-byte binary serial and unavailable
values. All four rs builds, 34 asset checks and 16 browser/USB programs pass.
App bytes are 1,963,984
classic, 1,965,376 both C3 profiles and 1,889,952 S3 in the unchanged
1,966,080-byte slots.

Owner-approved classic OTA from 0.1.25 succeeded with one upload of the
1,963,984-byte build from commit aef0efd. Wi-Fi, account, GPIO, room, Matter
pairing and the four assignments and OFF choices were retained. All stations
became reachable and returned serial numbers as text in the private device
information response. Live EN desktop (1280 px) and DE mobile (390 px)
verified the configured login hides the first-login hint. Both diagnostic
sessions were closed.

## rs-matter 0.1.25: Dangerzone setup details

The EN/DE browser regression at 1280/390/320 confirms Dangerzone omits the
current setup SSID and API key. The account-protected Wi-Fi & local API
section still shows both and copies the current key. Factory-reset preview
and copying the future password remain covered. All four rs builds and 34
asset checks pass. Classic firmware is 1,963,872 bytes in the 1,966,080-byte
slot. All 16 browser and USB programs pass. Owner-approved classic OTA from
0.1.24 to 0.1.25 succeeded on retry after the first upload connection failed
and the device was verified to still run 0.1.24. The live dashboard omits the
duplicate credentials and retains the network credentials and reset preview
controls. Home Wi-Fi, Matter pairing, account, room, GPIO bindings and all
four station assignments and OFF choices were retained. All four are
reachable. The diagnostic session was closed. A physical reset was not tested.

## rs-matter 0.1.24: reset preview and Matter platforms

Native maintenance checks cover the new preview action, strict confirmation
and key format, five-minute expiry, timer wrap and reset ordering. The
mocked EN/DE browser flow checks the two separate actions, current and
future password visibility, copying, cancellation, use of the displayed
key in the confirm request and retention of that key on the restart screen.
It also verifies the six Matter platforms and removal of the standalone
Alexa section. All four rs builds and 34 asset checks pass. Local app bytes
are 1,964,064 classic, 1,965,456 both C3 profiles and 1,890,064 S3. The app
slot is still 1,966,080 bytes. Real NVS reset and power interruption remain
unverified.

All 16 browser and USB programs pass. Owner-approved classic OTA from
0.1.23 to 0.1.24 succeeded with one upload of the locally checked
1,964,064-byte app from commit 93dc1e4. All four configured stations are
reachable and Matter remains ready and paired. The before/after snapshots
confirmed station settings, network, account, room and GPIO preservation.

Private on-device preview returned a distinct 24-digit hex key and left the
active key and account response unchanged. The running dashboard includes
all six Matter platforms and omits the separate Alexa panel. Diagnostic
sessions were closed. No reset confirmation request was sent.

## rs-matter 0.1.23: network names and station metadata

Default AP and hostname use the Bridge prefix. Custom device names still
override both defaults. The private debug dialog reads Lighthouse V2 metadata
on demand and displays absent fields as unavailable. The metadata source is
the V2 GATT mapping used by Lighthouse PM and the Bluetooth Device
Information Service.

Four complete rs builds and 34 Python package checks pass. Apps are
1,963,440 classic, 1,964,880 both C3 profiles and 1,889,536 S3 bytes in the
1,966,080-byte slots. The browser regression verifies the default names,
debug visibility, selected station and one read request, multiline firmware,
numeric model/serial values, a missing hardware revision, safe text rendering
and clearing after the dialog closes. The narrow dialog fits 320 px.

Owner-approved OTA from 0.1.22 succeeded with a single upload of the local
classic package from commit 5370cf1. The device serves 0.1.23, the updated
dashboard and the new default names through the account API. All four
stations report reachable. Matter is ready and remains paired. Saved
assignments and OFF choices, account credentials, network mode, room and
GPIO bindings were retained.

Read-only diagnostic jobs returned V2 type, Bluetooth name, firmware
blocks R/M/B, channel, manufacturer, numeric model and hardware revision
for all four physical stations. Station 2 responded on a later retry.
No job returned a serial number. The missing field does not establish
whether the raw characteristic is absent, unreadable or in an unhandled
format. Diagnostic sessions were closed. No station power or settings
writes were performed during metadata validation.

GitHub Actions run 44921519226 completed successfully. All four public
manifests and image SHA256s, chip/offset checks, exact partitions and OTA
payload equivalence were verified. Public app sizes are 1,963,216 classic,
1,964,640 both C3 and 1,889,312 S3 bytes.

## rs-matter 0.1.22: restored Identify behavior

The owner requests the 0.1.20 Identify behavior. Remove the 0.1.21 steady-ON
guard. The dashboard regression exercises Identify for a saved station
reporting SLEEP and checks the dedicated Identify request. All four rs
builds, native core and 34 asset checks pass. The classic app is 1,960,352
bytes in the 1,966,080-byte slot. All 16 browser and USB programs pass.

Owner-approved OTA from 0.1.21 to 0.1.22 succeeded with one upload of the
locally verified classic ESP32 package from commit 098e0f3. All four stations
are reachable and Matter is ready and paired. Station assignments and OFF
choices, network, account, room and GPIO settings were retained. The running
dashboard serves the restored Identify action. The diagnostic session is
closed. Physical Identify testing remains pending.

## rs-matter 0.1.21: Identify while already on

The owner observed that Identify on 0.1.20 could wake a station. The new
guard checks a recent successful steady-ON observation before queuing and
reads the power characteristic immediately before writing Identify.
Native regression coverage checks stale or failed observations, SLEEP,
STANDBY, starting values, malformed data and a failed read with partial
steady-ON data. The fake receives one LED write for confirmed steady ON and
zero power writes across the whole test. The targeted EN/DE browser test
covers refusal and a successful Identify job. All four rs builds and 34
Python asset checks pass. The classic app is 1,960,688 bytes in the
1,966,080-byte slot. All 16 browser and USB programs pass.

Owner-approved OTA from 0.1.20 to 0.1.21 succeeded with one upload of the
locally verified classic ESP32 package from commit 302614b. The device
reports Wi-Fi connected, Matter ready and paired, a usable BLE worker and
all four stations reachable. Station assignments and OFF settings, account,
room, network and GPIO settings were retained. The running device serves
the updated Identify message and the diagnostic session is closed. The
physical Identify behavior on a sleeping station remains pending.

## rs-matter 0.1.20: V2 Identify and Remote Mode

Simulated EN/DE browser coverage at 1280/390/320 checks Dangerzone placement
below the account section, Identify directly before Remove, suppression for
changed unsaved addresses, a matching Identify job result, and explicit
Remote Mode confirmation. No power, setup or station assignment requests
are issued by these flows. Native tests cover allowed wake pins, saved
binding requirements, individual wake, release debounce, aborted chords,
five-second exit and timer wrap on all three chip targets. Eight Rust
release tests pass. These checks do not exercise physical GPIO wake, LED
behavior, sleep current or command completion on the owner's device.

Stable and SDK regression builds pass. The rs firmware uses GCC size
optimization with one LTO partition to fit the additional code in both
existing OTA slots, keeping the pinned libraries and cache-off guards. All four
rs builds, Pages packaging and 34 Python checks pass. Local app sizes are
1,960,352 bytes for classic, 1,961,504 for both C3 profiles, and 1,886,656
for S3. The slot remains 1,966,080 bytes.

The earlier owner-approved OTA to 0.1.20 succeeded with a single upload of the locally
verified classic ESP32 package (1,960,352 app bytes, source commit 3d273da).
The device returned to home Wi-Fi with Matter ready/paired and BLE ready.
Two stations had readings at the initial check and all four were reachable
on follow-up. Pre/post comparisons retained network/account settings, room
name, assignments/OFF choices and GPIO bindings. The verifier initially
compared the whole GPIO response; checking the saved fields separately from
the new wake-pin list confirmed that the bindings were retained.

Live guarded browser checks passed in EN/DE at 1280/390/320. The Dangerzone
follows account settings, Identify is next to Remove on all saved stations,
and GPIO settings list the classic wake pins and disable activation while
the two saved bindings are disabled. The diagnostic session is closed.
Physical LED Identify and Remote Mode operation remain pending.

## rs-matter 0.1.19: room label and confirmed maintenance

Owner-approved classic OTA from 0.1.18 to **0.1.19** succeeded. One upload
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
