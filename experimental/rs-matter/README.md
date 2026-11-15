# Lighthouse Bridge — rs-matter 0.1.18

Adds two optional GPIO buttons and separate WebFlasher profiles for the classic
ESP32, WeAct ESP32-C3FH4, ESP32 C3 SuperMini and ESP32 S3 SuperMini (4 MB flash,
2 MB QSPI PSRAM). C3 uses a RISC-V Rust target; S3 uses its own Xtensa target.
Both app slots and the complete partition table stay unchanged. USB bootloader
offsets and OTA chip checks are specific to each chip. Never flash another chip's
image. New boards have not been tested on hardware. Owner-approved classic OTA
from 0.1.17 to 0.1.18 succeeded, retaining four assignments, OFF preferences,
credentials, naming and Matter pairing. Live EN/DE home-dashboard checks at
1280/390/320 pass; both GPIO bindings are disabled. Physical button operation
remains unverified. The owner authorizes this public release through the existing
Pages workflow. Ask before any further device change.

Software verification passes: all four full builds, stable/SDK regression
builds, 34 Python checks, 15 browser/USB programs, eight release Rust tests and
twenty native programs with GPIO/OTA chip variants. Final apps use 1,954,768
bytes (classic), 1,945,024 (both C3 profiles), 1,880,016 (S3), each below the
1,966,080-byte slot. Static RAM is 67,596 / 55,048 / 64,036 respectively;
these figures do not establish runtime memory margins or hardware operation.

Under **GPIO buttons**, assign two momentary contacts connected from GPIO to
GND. No external voltage is needed: firmware enables internal pull-ups. Choose
**All on**, **All off**, or **Disabled**, and separate supported GPIOs. Defaults
are disabled. Excluded GPIOs cover boot straps, flash/PSRAM, USB, serial and board
LED conflicts; consult the board's exposed pin labels before wiring. The BOOT
setup function remains separate (GPIO9 on C3, GPIO0 on ESP32/S3).

A press must remain stable for 40 ms. The input must be released after boot or
settings changes before it can fire; holding does not repeat. Buttons feed the
same sequential BLE queue as web/Matter commands, and All off respects each
station's SLEEP/STANDBY preference. Setup/OTA/restart gates reject presses without
replaying them afterwards. Configuration is stored atomically; invalid saved
bindings disable both buttons. No power command is sent automatically on boot.
Private GET/POST `/api/buttons` require an own-password session; POST also checks
Origin and JSON. API keys and bootstrap sessions cannot change GPIO assignments.

Build each profile with `python tools/build_rs_matter.py --board <profile>`:
`esp32`, `weact-c3`, `c3-supermini`, `s3-supermini`. Install the pinned ESP Rust
toolchain for all three chip targets. Packages are separated below `dist/rs-matter`
and Pages refuses mismatching chip/layout/hash/OTA data. C3's portable atomics use
the existing ESP-IDF critical-section implementation; pinned dependencies remain
locked. General source cleanup remains deferred.

# Lighthouse Bridge — rs-matter prototype 0.1.17

Version 0.1.17 refreshes the setup portal: two clear home/local choice cards,
short descriptions, compact network/branding details and a secondary Info/Leave
setup row. Local Mode confirmation/restart screens share the dark theme and
selected EN/DE language. Existing actions, authentication, mode selection and
clean setup behavior are retained; all assets are local.

The owner confirms Android setup access on 0.1.16. Owner-approved OTA to 0.1.17
is installed in home mode, retaining four assignments, OFF choices, credentials
and Matter pairing. Real EN/DE home-dashboard checks pass; the new portal was
not activated by the agent. The owner subsequently confirms the 0.1.17 portal
works on the device. Ask before future updates or setup/mode activation.
All thirteen browser/USB programs and 31 Python
checks pass, with EN/DE 1280/390/320 layout/navigation coverage. Final app:
1,950,896/1,966,080 bytes; static RAM 67,580. Slots and existing arena/task limits
remain intact. Preview screenshots use an example SSID; saved network names and
credentials are unchanged by this presentation update.

Version 0.1.16 opens a requested Wi-Fi setup through a clean AP-only reboot,
before Matter/BLE initialization. The request is consumed once; the saved home/
local mode, router credentials, own password, API key, stations and pairings stay
intact. Setup timeout/close returns to the saved mode. The browser explains the
restart and keeps the API key copyable until the owner reconnects.

This targets an owner-reported Android setup-password rejection despite a verified
API key; the cause is not established. Espressif documents unstable SoftAP/BLE
coexistence on classic ESP32. Isolating setup removes competing initialized stacks,
and the owner now confirms Android setup access. The original failure cause
and permanent Local Mode/BLE behavior remain unproven.

For diagnosis send LH_WIFI_INFO over physical USB. Its response contains only
numeric mode/auth/cipher/channel/PMF/config status, key/name match booleans,
client/event counts, last disconnect reason and byte-heap values. It includes no
actual keys, SSIDs or MACs. AP startup and numeric connection events also log
non-secret diagnostics. Existing USB recovery commands are unchanged.

All 18 native programs, eight Rust tests in debug/release, 31 Python checks and
thirteen browser/USB programs pass, including exactly-once setup persistence and
EN/DE key-copy/restart handoff. Stable/SDK builds also pass. Final app:
1,942,832/1,966,080 bytes, static RAM 67,588; slots and arena/task limits stay.
These results do not prove Android connectivity or AP/BLE runtime behavior.

Owner-approved OTA from 0.1.15 to 0.1.16 is installed in home mode, retaining
four assignments, OFF choices, credentials and Matter pairing. Real EN/DE home-
dashboard checks pass, including the new setup confirmation (cancelled). Setup
was not activated by the agent; the owner subsequently confirms Android setup
access. Permanent Local Mode/BLE remains a hardware check. Ask before
subsequent updates or device actions. No password reset or key rotation occurred.

Local Mode in version 0.1.15 provides a permanent private Wi-Fi network without
an external router. Open WiFi Setup (dashboard button or hold BOOT for five
seconds), connect to the protected setup network, choose Local Mode and confirm.
After restart, reconnect to the same bridge Wi-Fi and open http://192.168.4.1/.
Its Wi-Fi password is the existing API key; normal web login still uses the own
password. Keep the Wi-Fi connection even if your phone reports no internet.

The selection persists across restarts. BLE station controls, discovery/editor,
read-only updates, local API and personal-session OTA remain available. Matter
is paused in this mode; Alexa on a separate home network cannot reach the bridge.
Pairings, endpoints, station settings and router credentials stay stored. Use
Return to home Wi-Fi in the dashboard to restart into the saved home network, or
WiFi Setup to select/configure one. Without a reachable saved network, setup opens.
No power command is sent automatically during mode switches or restarts.

Changing the AP name or regenerating its key restarts the AP to apply the new
settings. Copy the new key from the still-open page before reconnecting; the web
password is unchanged. Credential/mode changes require your personal session.
Initial Local Mode selection requires the protected setup portal and confirmation.

All 18 native programs, eight Rust tests in debug/release, 31 Python checks and
12 browser/USB programs pass, including EN/DE mobile-emulated local workflows,
persistence failure, handoff/privacy and paused Matter controls. Stable/SDK builds
also pass. App 1,940,736/1,966,080 bytes, static RAM 67,572; both slots and existing
arena/task limits are retained. Owner-approved OTA to 0.1.15 is installed in home
mode. Four assignments, OFF preferences, credentials and Matter pairing are
retained; real EN/DE desktop/mobile dashboard checks pass. No network-mode switch
was performed. Ask before subsequent updates or changing the actual mode.
Standalone AP/
BLE coexistence, AP OTA and a home/AP round-trip still need hardware validation.

Version 0.1.14 reverts the language selector's black-on-light override to its
preceding theme appearance, preserving size and debug-only manual refresh.
Owner-approved OTA to 0.1.14 succeeded; four assignments, OFF preferences,
credentials and Matter pairing are retained. Real EN/DE desktop/mobile checks
confirm the restored appearance and debug-only refresh. Both targeted
browser programs and all 31 Python checks pass; app 1,930,352/1,966,080 bytes,
static RAM 67,540 unchanged.

Version 0.1.13 uses black text on a light language selector and shows Read
current states only in debug mode. Background reads stay active; changing debug
visibility sends no read or power request. Owner-approved OTA to 0.1.13
succeeded, preserving four assignments, OFF choices, credentials and Matter
pairing. Real EN/DE desktop/mobile browser checks confirm both changes. Ask
before subsequent device updates. Three targeted browser programs
and all 31 Python checks pass. App 1,930,368/1,966,080 bytes; static RAM 67,540
and existing partition/arena/task limits are unchanged.

Version 0.1.12 adds optional username settings and confirmed API-key regeneration
under the authenticated account/network sections. First setup needs no username;
existing accounts stay password-only until enabled. Username settings preserve
the password and invalidate other sessions. Regenerating the key invalidates old
API integrations and changes the setup-AP password, preserving router Wi-Fi.

Per-station OFF preference defaults to SLEEP and optionally uses STANDBY (motor
keeps running). Alexa and dashboard OFF use the saved choice; local API state off
also follows it, while explicit sleep/standby/on states remain exact. No power
command is sent automatically on boot, reconnect, scan or configuration change.

Connection status moves right of branding; language and Sign out controls match.
Cards show Alexa names first, with ID/address/history only in debug and adaptive
card height. Mobile discovery uses a native select plus manual address input.
Owner-approved OTA is installed: four assignments, credentials and Matter pairing
are retained, OFF defaults stay SLEEP and username is disabled. Real EN/DE browser
checks pass desktop/mobile widths, adaptive debug cards and matching header controls.
An existing 0.1.11 browser tab automatically reloaded 0.1.12 after external OTA.
Actual standby writes and key/username changes were not performed during these
checks; they are covered by native/browser software tests. Ask before subsequent
device changes. General cleanup and public publication remain separate.

All 17 native programs, eight Rust tests in debug/release, 31 Python tests and
11 browser/USB programs pass. Stable PlatformIO also builds. The app occupies
1,930,336/1,966,080 bytes, static RAM 67,540; arena/task/partition limits are
unchanged. These checks do not establish long-term or sixteen-station margins.

The local 0.1.11 version uses Lighthouse-Bridge as its display/product and
commissioning name. Existing assignments, station names, endpoint/UniqueIDs,
fabrics, credentials and hostname are retained. The old test-name suffix is
removed; test attestation remains test attestation.

Version 0.1.11 originally placed connection status left of the top branding;
0.1.12 moves it right. Measured ON retains green,
Unknown uses gray, SLEEP blue and STANDBY orange; requested targets do not alter
the measured color. Text/contrast and existing fixed status dimensions remain.

An acknowledged browser OTA waits for the reachable rebooted login gate, then
reloads once. Old authenticated responses do not count as reboot completion,
failed uploads do not reload, and reconnect attempts never re-upload. A 90-second
deadline provides a manual-reload fallback. A firmware change initiated outside
the tab is detected through status version/public page version after session
loss; normal session expiry does not reload unchanged firmware. Load the new
HTML once to enable this behavior; an existing old tab needs an initial refresh.

The owner-approved OTA from 0.1.10 to 0.1.11 succeeded, retaining four assignments,
credentials and Matter pairing. Real EN/DE browser checks pass at 1280/390/320 px:
status position, no horizontal overflow, blue measured SLEEP badges and the
Lighthouse-Bridge setup name. No new pairing window or power command was sent.
Ask before future device changes; public publication is not included.
No general cleanup or BLE/power/identity/partition/stack change is included.
The app is 1,925,824/1,966,080 bytes (+480 over 0.1.10); static RAM
remains 67,524 bytes. Eight Rust tests pass in debug/release, 31 Python checks
and all ten browser/USB programs pass. EN/DE layout/color/contrast and simulated
OTA reconnect/reload checks pass at desktop and 390/320-pixel mobile widths.
The new OTA reload flow and colors other than the four actual SLEEP readings
are software-tested. An old tab needs one initial refresh; long-term and
sixteen-station runtime margins remain unverified.

Version 0.1.10 simplifies Matter Setup. Explicitly opening the
section reads its private status and displays an active code directly. The
separate Show active code/status button is removed; New code & open pairing
window remains the only pairing mutation action. Closing clears the code and
stops polling, including late responses. Logout closes the section, requiring
another explicit opening after login. The copy button and EN/DE behavior remain.
The owner-approved OTA from 0.1.9 to 0.1.10 succeeded, retaining four assignments,
credentials, naming and Matter/worker readiness. Real Chromium checks on the
board confirm EN/DE automatic reads on explicit opening, correct paired/closed
status, clearing on collapse/reopening, and no separate show control. No new
pairing window or power command was sent; active-code behavior is covered by
software tests. Ask before every subsequent device change. General code cleanup
and protocol/BLE/identity changes are not included in this UI update.
The app uses 1,925,344/1,966,080 bytes (+64 over 0.1.9) with unchanged
67,524 static RAM bytes. Eight Rust tests in debug/release, 31 Python checks and
all nine browser/USB programs pass. These results do not establish long-term or
sixteen-station runtime margins.

The deployed 0.1.8 bridge was queried directly over its LAN using an actual PASE
handshake and encrypted Descriptor reads. Root PartsList was `[2,3,4,5,6]`,
aggregator 2 listed `[3,4,5,6]`, and endpoint 1 returned UnsupportedEndpoint.
The four station endpoints correctly reported On/Off Light + Bridged Node.
The probe closed its diagnostic session; firmware, fabrics and assignments were
retained, with no reboot or power command.

[matter.js ecosystem documentation](https://github.com/matter-js/matter.js/blob/main/docs/ECOSYSTEMS.md)
records Alexa's non-standard requirement for endpoint 1 as the main device or
an aggregator. The earlier host fixture included a legacy light at 1 and missed
this actual deployed topology. A new encrypted runtime regression fails with
exactly the deployed `1/29/0 UnsupportedEndpoint` before the correction.

The candidate adds a Descriptor-only aggregator at 1 when no active station owns
that endpoint. Existing lights retain their endpoint IDs and UniqueIDs. The old
aggregator 2 remains present but empty in that case; aggregator 1 lists only the
station endpoints. If a legacy light already owns 1, it remains a light and
aggregator 2 keeps its original role. Each station has exactly one aggregator.
Descriptor DataVersions are randomized once per boot so retained controllers
cannot filter a changed PartsList using the former fixed version 1.
Zero to sixteen stations remain supported. Metadata is sorted and allocated once
for the actual count, up to 19 endpoints, without reallocating during construction.

The owner approved the narrow exception that assigns currently unbound endpoint
1 an aggregator role. The old registry does not distinguish never-used from
retired 1. No saved station binding, allocator counter, Wi-Fi, password, fabric or
partition is reset. This does not authorize further endpoint reuse. Ask before
every subsequent device change, firmware write, reset or new pairing action.

Host coverage checks encrypted individual, reversed, repeated and batched reads,
actual serialized device types/PartsList/ServerList, and multi-packet ReportData
acknowledgements. Isolated production runtimes cover a legacy light at 1, the
reported five-station topology, and missing-1 configurations with four and sixteen
stations. These fixtures seed established CASE sessions; they do not exercise
CASE establishment or Alexa. The deployed 0.1.8 PASE probe is real hardware
read-only evidence. Owner-approved OTA to 0.1.9 succeeded on the real board;
Matter/worker readiness, the four assignments, API key, web password and device
naming were retained after reboot, with command histories still none. The owner
then approved a new pairing window and direct inspection. Real encrypted PASE
reads on 0.1.9 confirm aggregator 1 with station children 3..6, retained empty
aggregator 2, root PartsList [1,2,3,4,5,6], and unchanged station types. There are
no UnsupportedEndpoint errors, reboot or power commands during that probe.
The owner confirms complete Alexa pairing with 0.1.9. A separately requested
real-browser All to SLEEP test transitions all four stations from measured ON
to measured SLEEP, reachable and command-confirmed. Alexa app synchronization
for that transition still awaits the owner's observation; sixteen-station and
long-term operation are not established by these four-station results.

Keep the numeric startup inventory and bounded rejection-path diagnostics.
Task stacks remain 18 KiB foreground / 8 KiB BLE, with a 14 KiB future arena and
its exact-type size/alignment + 1 KiB compile guard. No automatic power commands
or wider flash slots are introduced.

This replaces the Tasmota experiment with the independent Rust implementation
`project-chip/rs-matter`, pinned to 43f61585a439f1a1e90924b696a6280850ef0145.
The existing production C++ dashboard, authentication, API, OTA receiver and
single NimBLE worker remain shared with stable firmware. No CHIP C++ Matter
library, Tasmota runtime or Berry interpreter is linked into this prototype.

Hardware: classic ESP-WROOM-32 / ESP-32S, at least 4 MB flash. The exact existing
partition table and both 1.875 MiB app slots are mandatory. No merged USB image.
Do not flash firmware for S2/S3/C3 to this board.

Commissioning uses the existing saved Wi-Fi connection, then Matter over IP.
Bluetooth remains dedicated to Lighthouse V2. Commands queue ON (0x01), SLEEP
(0x00) or explicitly selected STANDBY (0x02); boot, reconnect, scans and periodic
reads never send automatic power.
The Matter state and Reachable attributes use actual measurements. Up to 16
configured stations have distinct bridged On/Off Light endpoints. Their existing
registry under lhmatter/endpoints and custom names are retained.

rs-matter's fabrics, ACLs and other protocol state use a separate lhrsmatter NVS
namespace. Existing Tasmota and CHIP fabric formats are not imported. Switching
protocol implementations requires pairing this prototype with Alexa again; it
must not erase Wi-Fi, the API key, the web password or station assignments. Old
protocol storage is left intact. The test bridge name remains exactly
Lighthouse-Bridge in version 0.1.11. Test attestation is not product certification.

The authenticated dashboard shows the active manual code only on explicit access.
New code & open pairing window generates a new local 15-minute pairing window.
Codes expire with the window and clear on logout/restart. API keys and bootstrap
sessions cannot retrieve them. The setup action does not remove existing fabrics.

Build with the pinned Arduino ESP32 3.3.12 and library versions in
`tools/build_rs_matter.py`, Espressif Rust 1.97.0.0 (espup 0.19.0) and the committed
Cargo.lock. Run `python tools/build_rs_matter.py` after sourcing espup's export.
Outputs are in dist/rs-matter: four split ESP32 images, .lhota, SHA256SUMS and
source ZIP. Future OTA uses only the separate .lhota through a personal-password
web session. Settings-preserving USB updates must leave erasing unchecked.

Build and hardware validation status is recorded in docs/VALIDATION.md. A build
or simulated test does not establish successful Alexa pairing, runtime RAM margins,
actual OTA or prolonged multi-station Bluetooth/Wi-Fi coexistence.

The 0.1.9 baseline validation: 1,925,280/1,966,080 app bytes; 67,524 static RAM
bytes. Eight Rust host tests pass in debug and release, sixteen C++ programs,
31 Python package checks and all nine browser/USB simulations pass. The exact
future-arena guard and ELF checks retain 14 KiB arena / 52,528-byte Stack object /
18 KiB CPU task stack. These are software/build results, not candidate hardware
RAM margins or a successful Alexa pairing. The separate real OTA result above
is limited to one update on the owner's board.

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
