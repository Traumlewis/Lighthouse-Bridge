# Version history

The active development and WebFlasher baseline is LH-Bridge 0.1.30.
Versions 0.1.31 through 0.1.33 are historical setup experiments.

## Setup address — LH-Bridge 0.1.33

- Show the setup portal address in the WebFlasher's USB panel.
- Advertise the setup DNS server to Wi-Fi clients and bind the setup
  HTTP listener to the access-point address.

## Setup portal HTTP — LH-Bridge 0.1.32

- Use the bounded dashboard HTTP response writer for setup pages.
- Report the setup listener and attempted-response count in `LH_WIFI_INFO`.

## Initial Wi-Fi setup — LH-Bridge 0.1.31

- Switch directly to AP mode on first setup to avoid restarting the Wi-Fi
  driver. Add redacted USB diagnostics for the portal and AP IPv4 state.

## Seeed Studio XIAO boards — LH-Bridge 0.1.30

- Add WebFlasher and OTA profiles for Seeed Studio XIAO ESP32-C3 and
  XIAO ESP32-S3, with the same partition table and board-specific GPIO options.
- Use 8 MB flash and OPI PSRAM for the standard XIAO ESP32-S3. Check the
  flash-size header when accepting S3 OTA packages.

## Unified version label — LH-Bridge 0.1.29

- Show `LH-Bridge 0.1.29` in the WebFlasher, dashboard, boot banner and Matter
  software version string. Use `0.1.29` in manifests, protocol version fields
  and download filenames.

## Main Lighthouse Bridge firmware — 0.1.28-rs-matter

- Publish rs-matter as the project firmware for all four board profiles.
- Put board selection, USB setup and downloads on the main WebFlasher.
- Retire the Hue, SDK Matter and separate experiment installers and build paths.

## Automatic reload after restart — 0.1.27-rs-matter

- Dangerzone restart reloads the dashboard once the bridge is available again.
- The classic OTA and one physical restart were verified. The served dashboard
  reloaded to sign-in in the simulated reconnect check.

## Login and station serial number — 0.1.26-rs-matter

- Show the USB/API-key login instructions only before a web password is set.
- Display printable Bluetooth serial numbers in Device information.
- Owner-approved classic OTA and the configured login screen were verified.
  Device information returned serials for all four physical stations.

## Dangerzone setup details — 0.1.25-rs-matter

- Show the current setup Wi-Fi name and API key in Wi-Fi & local API.
  Dangerzone shows the future password when preparing a factory reset.
- Owner-approved classic OTA was verified. All four stations are reachable
  and Matter remains paired.

## Reset password preview and smart home — 0.1.24-rs-matter

- Show the next API key and setup Wi-Fi password before factory reset.
  Confirmation applies the displayed key after the standard NVS erase.
- Combine Matter setup and smart home platforms in one panel. The list
  identifies Matter controllers and marks Alexa as tested with this bridge.

- Owner-approved classic OTA succeeded. Live verification confirmed the
  preview key is distinct and leaves the active key in place.

## Bridge network names and station diagnostics — 0.1.23-rs-matter

- Default setup Wi-Fi is `Lighthouse-Bridge-Setup-XXXXXX` and the default
  hostname is `lighthouse-bridge-xxxxxx`.
- Read V2 device information on demand from a station's debug card, including
  its Bluetooth name, firmware revisions, channel, manufacturer, model,
  serial number and hardware revision when available.

- Owner-approved classic OTA was verified. Read-only metadata jobs succeeded
  for all four stations. Serial numbers were unavailable in these results.

## Identify on sleeping stations — 0.1.22-rs-matter

- Restore the 0.1.20 Identify behavior at the owner's request. The dedicated
  LED command can wake a sleeping station. The bridge sends no separate power
  command.

- Owner-approved OTA is installed on the classic ESP32. All four stations
  are reachable and Matter remains paired.

## Identify requires an already running station — 0.1.21-rs-matter

- Prevent Identify from waking sleeping stations. Require a recent successful
  steady-ON measurement before queuing, and confirm steady ON over Bluetooth
  immediately before sending the LED command.
- Owner-approved classic OTA is installed. All four stations are reachable
  and Matter remains paired. Physical Identify verification is pending.

## Identify and Remote Mode — 0.1.20-rs-matter

- Move Dangerzone below the account section. Identify a saved Lighthouse V2
  from station setup using its LED.
- GPIO settings offer Remote Mode with two wake-capable button bindings.
  A single press followed by release runs All on or All off via BLE. The
  bridge returns to deep sleep after the jobs finish. Hold both buttons for
  five seconds to return to the previous Wi-Fi mode.

- Owner-approved classic OTA from 0.1.19 succeeded and preserved the four
  assignments, GPIO settings and Matter pairing. Live EN/DE UI checks passed.
  Physical LED and GPIO tests remain pending.

## Room name and maintenance — 0.1.19-rs-matter (public release; installed on classic ESP32)

- Click the room heading to save a personal room/owner label (up to 64 UTF-8
  bytes). The label survives reboot and is independent of the network hostname.
  Hide the introduction heading and paragraph when a station is configured.
- Add a private Dangerzone with the setup Wi-Fi name/password (click to copy),
  Restart and explicitly confirmed Factory reset. Restart retains settings.
- Factory reset removes all NVS settings, including home Wi-Fi credentials,
  API key, web authentication, room name, station/endpoint mappings, buttons and
  Matter fabrics. Read the newly generated setup password over physical USB,
  then configure Wi-Fi/login/stations and pair Matter again. Existing app slots,
  the complete partition table and OTA boot selection are not erased.
- Maintenance requires a personal-password session and same-origin JSON. API
  keys/bootstrap sessions cannot use it. Reject scans, pending commands, setup
  and OTA; stop new BLE work and wait for physical idle before reboot. A durable
  reset flag is applied before network/BLE/Matter initialization on the next boot.
  No power commands are sent automatically. Owner-approved classic OTA from
  0.1.18 succeeded with one upload of the published package. Assignments, OFF
  choices, credentials, naming, GPIO settings and Matter pairing are retained.
- Verification passes: four full rs builds plus stable/SDK regressions, all native
  CI checks, eight Rust tests in debug/release, 34 Python checks and 16 browser/USB
  programs. Local classic app 1,959,440/1,966,080 bytes; installed public app 1,959,216.
  Live EN/DE device UI at 1280/390/320 passes. No actual factory-reset, separate
  restart or room save was performed; those remain software-tested.

## GPIO buttons and board profiles — 0.1.18-rs-matter (public release; installed on classic ESP32)

- Add two configurable GPIO-to-GND buttons, disabled by default, for All on/All
  off. Debounce and release-to-arm prevent startup/held-button commands. Reuse
  the existing queue and per-station OFF preference.
- Add private session-protected GPIO settings in EN/DE, conservative pin lists,
  duplicate/invalid-pin rejection and atomic persistence.
- Add separate ESP32, WeAct ESP32-C3FH4, C3 SuperMini and S3 SuperMini (4 MB flash,
  2 MB PSRAM) builds and WebFlasher selection. Validate chip-specific USB/OTA
  images while retaining both app slots and the complete partition table.
- Four builds, stable/SDK regression builds, 34 Python checks, 15 browser/USB
  programs, eight release Rust tests and twenty native programs (including
  GPIO/OTA chip variants) pass. All apps fit the unchanged slot. New GPIO wiring
  and C3/S3 runtime operation remain unverified.
- Owner-approved classic OTA from 0.1.17 succeeds; four assignments, OFF choices,
  credentials and Matter pairing remain intact. Live EN/DE checks confirm ready
  Matter/BLE, responsive home dashboard and default-disabled GPIO bindings.
  No power/config/commissioning action or publication was performed.

## Clearer Wi-Fi setup portal — 0.1.17-rs-matter (local, not published)

- Present home Wi-Fi and Local Mode as separated choice cards with explanations.
  Move Info/Leave setup into a secondary row and use compact branding/SSID copy.
- Replace the expected home-disconnected status on the landing page with clear
  setup instructions. Keep real configuration failures visible on vendor pages.
- Match confirmation/restart styling and carry EN/DE language selection through
  the local flow. Use local assets only; retain all existing routes and protections.
- All thirteen browser/USB programs and 31 Python checks pass, with synthetic
  EN/DE desktop/mobile previews. The app fits the unchanged OTA slot.
- Owner-approved OTA from 0.1.16 succeeds in home mode, retaining assignments,
  OFF choices, credentials and Matter pairing. Real EN/DE home-dashboard checks
  pass; the agent did not activate the new setup portal. Android setup access
  was owner-confirmed on 0.1.16; the owner also confirms the new 0.1.17 portal works.

## Clean setup reboot and AP diagnostics — 0.1.16-rs-matter (local, not published)

- Open Wi-Fi setup on a fresh AP-only boot before initializing Matter/BLE, using
  a consumed-once setup flag. Retain the saved home/local choice and all settings.
- Preserve protected setup, confirmation and timeout/return behavior. Keep the
  API key copyable during the browser handoff and explain the restart in EN/DE.
- Add non-secret physical-USB AP configuration/connection diagnostics, including
  whether the driver key matches the API key and numeric disconnect reasons.
- The owner's API key was verified correct after an Android setup rejection.
  The owner subsequently confirms setup access on 0.1.16; the original cause
  remains unproven and permanent Local Mode/BLE is not established by that test.
  All 18 native programs, eight Rust tests in debug/release, 31 Python checks
  and thirteen browser/USB programs pass. App fits the unchanged OTA slot.
  Owner-approved OTA from 0.1.15 succeeds, preserving home mode, assignments,
  credentials and Matter pairing. Real home-dashboard checks pass; clean setup
  was not activated by the agent. The owner subsequently confirms successful
  Android setup access; permanent Local Mode/BLE still needs testing.

## Permanent Local Wi-Fi Mode — 0.1.15-rs-matter (local, not published)

- Add WiFi Setup -> Local Mode -> Webinterface at 192.168.4.1, using the bridge's
  protected Wi-Fi without a router. Persist the choice across reboots.
- Keep measured station control/discovery/configuration, sessions, local API and
  OTA available. Pause Matter/Alexa until returning to home Wi-Fi; retain pairings,
  Wi-Fi credentials, station identities, passwords and OFF preferences.
- Add confirmed return to saved home Wi-Fi. Apply AP key/name changes by restart,
  keeping the new password copyable until reconnect/reload.
- Expose local-mode address through USB and distinguish paused Matter in EN/DE.
- All 18 native programs, eight Rust tests in debug/release, 31 Python checks
  and twelve browser/USB programs pass. Both existing slots are retained.
  Owner-approved OTA from 0.1.14 succeeds in home mode, preserving assignments,
  settings and Matter pairing. Real EN/DE desktop/mobile dashboard checks pass.
  AP activation and actual AP hardware operation remain untested.

## Restore language selector theme — 0.1.14-rs-matter (local, not published)

- Revert the black-on-light language selector override, restoring the preceding
  theme appearance. Keep Sign out sizing and debug-only manual state reads.
- Two targeted browser programs and 31 Python checks pass; app fits unchanged
  slots with unchanged static RAM. Owner-approved OTA from 0.1.13 succeeds,
  preserving assignments, OFF preferences, credentials and Matter pairing.
  Real EN/DE desktop/mobile browser checks confirm the restored appearance.

## Language contrast and debug controls — 0.1.13-rs-matter (local, not published)

- Use black text on a light background for the language selector and its options.
  Preserve its existing size and keyboard behavior.
- Show Read current states only in debug mode. Automatic BLE reads are unchanged;
  toggling debug does not queue work. Logout hides the control again.
- Targeted browser tests and 31 Python checks pass; app fits the unchanged OTA
  slot with unchanged static RAM. Owner-approved OTA from 0.1.12 succeeds,
  preserving four assignments, OFF settings, credentials and Matter pairing.
  Real EN/DE desktop/mobile browser checks confirm both requested changes.

## Account settings, OFF preferences and mobile discovery — 0.1.12-rs-matter (local)

- Put connection status right of the bridge name and match language-select sizing
  to Sign out. Use Base station in English; preserve saved Alexa names.
- Show Alexa name first in station cards. Reveal station ID, Bluetooth address
  and technical history in debug only; cards expand/shrink with debug mode.
- Use a native discovered-device select on mobile while retaining manual MAC entry.
- Persist SLEEP/STANDBY OFF preference per station, used by Alexa and dashboard.
  Existing stations default to SLEEP; exact measured target confirms commands.
- Add optional username enable/disable/change under password and username settings.
  First setup and existing accounts default to password-only. Keep password hash
  and username settings in one atomic record; changing one preserves the other.
- Allow confirmed API-key regeneration for personal sessions. Old integration keys
  stop working; setup-AP password changes, router Wi-Fi and web password stay.
- Owner-approved OTA succeeded. Four assignments, access settings and the Alexa
  pairing are retained; real desktop/mobile UI and external-OTA automatic reload
  checks pass. Credentials and OFF choices were preserved during verification.
  Optional username/key changes and standby commands are covered in software.

## Dashboard colors, naming and OTA reload — 0.1.11-rs-matter (local, not published)

- Use Lighthouse-Bridge as the rs-matter display/product/commissioning name.
  Keep saved station names, IDs, fabrics and custom hostname intact. This changes
  a name, not test attestation or certification status.
- Move connection status left of the top branding. Keep ON green; use gray for
  Unknown, blue for SLEEP and orange for measured STANDBY, with readable text.
  Pending targets never override measured badge colors.
- After successful browser OTA, wait for the rebooted login endpoint and reload
  once, without repeating the upload. Detect external firmware changes too;
  ordinary session expiry only returns to login. Reconnect waiting is bounded
  with a manual-reload fallback. The new HTML must be loaded once to enable this.
- Owner-approved OTA from 0.1.10 succeeded, retaining four assignments, access
  settings and the existing Matter pairing. Real EN/DE browser checks pass at
  desktop/mobile widths; no pairing window or power command was issued. The new
  OTA reload flow is software-tested and needs the new HTML loaded once.
  No general cleanup, power-policy or partition/stack changes.

## Simpler Matter Setup — 0.1.10-rs-matter (local, not published)

- Opening Matter Setup now reads the private pairing status and any active code
  directly. Remove the separate Show active code / pairing status control.
  Keep New code & open pairing window as the sole pairing mutation action.
- Closing the section stops polling and clears the code. Ignore late responses
  while closed; reopen with a fresh read. Logout closes the section so the next
  login cannot reveal a cached or newly fetched code implicitly. Preserve copy,
  EN/DE translations, renewal queueing and non-flickering background updates.
- No change to Matter endpoints, fabrics, BLE commands, stacks, packet capacities
  or flash layout. The build adds 64 app bytes; static RAM is unchanged.
- Owner-approved real OTA from 0.1.9 to 0.1.10 succeeds, retaining four station
  assignments, credentials, naming and Matter/worker readiness. Actual EN/DE
  dashboard inspection confirms automatic status reads and no separate show
  control. No new pairing window or power command was sent during verification.
  Ask for approval before every further device change.

## Rust prototype Alexa endpoint 1 — 0.1.9-rs-matter (local, not published)

- Direct encrypted LAN reads reproduce the deployed missing endpoint 1:
  root PartsList [2,3,4,5,6] and UnsupportedEndpoint for 1/29/0. Alexa's documented
  endpoint-1 compatibility requirement was absent from earlier test fixtures.
- Add a Descriptor-only aggregator at 1 when no active light owns that endpoint.
  Retain aggregator 2 as empty and preserve all active light endpoints/UniqueIDs.
  Legacy light 1 keeps its existing role and aggregator 2. Each station belongs
  to one aggregator. Keep the registry, fabrics, credentials and exact slots.
- Randomize Descriptor DataVersions once per boot so old controller filters
  cannot hide a changed PartsList after restart.
- Cover missing-1 runtimes with four/sixteen stations, real encrypted reports,
  topology/type payloads and chunk acknowledgements. No automatic power commands.
- The owner approved the endpoint-1 role exception and real OTA from 0.1.8 to
  0.1.9 succeeded, preserving four assignments, credentials, naming and readiness
  with no power commands. A separately approved pairing window and real encrypted
  PASE inspection confirm working aggregator 1, unchanged stations 3..6 and
  retained empty aggregator 2 without endpoint errors. Ask before every later
  device change. The owner confirms successful Alexa pairing. A separately
  authorized real dashboard All to SLEEP test transitions four measured ON
  stations to measured, reachable, confirmed SLEEP; the Alexa app state update
  for that transition still awaits the owner's observation.

## Rust prototype rejection-path diagnostics 0.1.8-rs-matter

- Log the actual sorted endpoint IDs at startup and bounded numeric paths for
  SDK attribute/command rejections: operation, endpoint, cluster and attribute
  or command ID. Preserve existing fixed error/event labels. Raw SDK messages,
  node/fabric identities, names, credentials and packet bodies stay excluded.
- Parse only the pinned SDK's typed status-path format, reject malformed or
  overflowing numeric fields, retain the 256-byte formatter and 32-event queue,
  and defer all output until protocol polling has returned and locks are released.
- Owner 0.1.7 still reports GS014 despite CASE and CommissioningComplete
  persistence. An UnsupportedEndpoint read remains, without its request path.
  This diagnostic release does not claim to fix GS014. The endpoint-order
  correction, identities, settings, capacities and both app slots remain.
- The owner measured only 692 bytes left on the foreground task stack. Move
  2 KiB from the separate compile-checked Rust future arena to that task stack:
  ESP32 arena 14 KiB, task stack 18 KiB, still 32 KiB together. Keep the exact
  future size/alignment + 1 KiB compile guard, host arena and BLE stack unchanged.
  Runtime margins and Alexa acceptance remain hardware checks.

## Rust prototype endpoint lookup correction 0.1.7-rs-matter

- Sort the Matter metadata endpoints by their existing IDs. The previous order
  `0, 2, 1, 3, ...` violated the pinned SDK's Node contract and its resumable
  Interaction Model path expander's binary search. This changes metadata order
  only; persisted endpoint IDs, station bindings and per-station state remain intact.
- Exercise real parsed Descriptor reads through the SDK, for each endpoint and
  a wildcard, with full, sparse, shuffled and empty configurations. Check debug
  and release host profiles, including a negative legacy-order fixture.
- Owner 0.1.6 evidence confirms CASE and CommissioningComplete persistence,
  followed by an UnsupportedEndpoint read. The endpoint lookup defect is
  reproduced in software; Alexa app acceptance with this correction still needs
  hardware confirmation. Both app slots, all capacities, stacks and BLE/power
  behavior are unchanged.

## Rust prototype commissioning diagnostics 0.1.6-rs-matter

- Add bounded, privacy-filtered upstream protocol diagnostics for attestation,
  AddNOC, CommissioningComplete, RemoveFabric, fail-safe expiry and CASE/exchange
  errors. Only fixed event/error labels leave the Rust adapter; raw SDK log text,
  packet data, credentials, certificate material and device identities are excluded.
- Log public-state changes in fabric count, active PASE/CASE sessions, fail-safe
  and window state. A saved fabric is not proof of completed Alexa commissioning.
  Diagnostic serial output runs after protocol polling and outside stack locks.
- Owner 0.1.5 logs now show receive progress, but Alexa still reports GS014.
  A fresh attempt removes the previously retained fabric without any logged heap
  allocation failure. Its cause is unresolved; this release adds evidence and
  does not claim a hardware pairing fix. Settings, BLE policy and both app slots
  are preserved.

## Rust prototype UDP receive-readiness correction 0.1.5-rs-matter

Fix the adapter's NetworkReceive::wait_available contract: it previously returned
success after 5 ms even for an empty socket. The Matter receive task then held
the shared RX buffer while waiting for UDP/5540, blocking ready mDNS/5353 input.
This matches the owner's four initial mDNS sends followed by zero received UDP.
The defect reproduces in the full Rust stack with a queued DNS PTR query: old
adapter consumes zero packets, corrected adapter consumes the query and emits a
Basic Commissioning DNS response. Previous send-only host probes missed this.

Use a nonblocking, non-consuming MSG_PEEK probe and wait only while no datagram
is available, before the stack acquires RX storage. Treat zero-length UDP as
readable and propagate fatal socket errors. Native socket tests cover empty,
ready, repeated peek, full payload retention, zero-length and closed sockets.
Preserve stack sizes, Wi-Fi bounds, protocol capacities, NVS and flash layout.
Actual Alexa pairing and the effect on hardware heap pressure still need testing.

## Rust prototype bounded Wi-Fi buffers and UDP backpressure 0.1.4-rs-matter

Owner 0.1.3 evidence has no panic, but over 24,000 allocation failures, recurring
BLE_NO_MEMORY and heap minima around 180 bytes. Alexa reports device not found
while the commissioning window is open. This does not establish a single
allocator caller or successful delivery of discovery packets.

Bound the public Wi-Fi init config after Arduino's hard-coded defaults: dynamic
TX 32 -> 4, RX 32 -> 8 for this board, preserving static RX and aggregation minima
and all unrelated config fields. Link through esp_wifi_init's public argument,
without modifying SDK sources or private layouts. Log applied limits at startup
and require the wrapper in the final ELF. This reduces possible buffer bursts,
with lower peak throughput; it is not a proven Alexa fix or RAM margin.

Handle temporary UDP ENOMEM/ENOBUFS/EAGAIN/EINTR with 10 ms asynchronous pauses
for up to 1.5 seconds; timeout/permanent/short sends fail, never fake ACK or split
a datagram. Cancellation keeps the socket usable. Log UDP TX/RX/retries/errors,
timeouts, mDNS socket TX/RX and IPv6 address count without payloads/identifiers.
Counters prove only local socket activity, not Echo receipt or packet parsing.
Keep protocol/station capacity, stacks, NVS, partitions and the measured BLE
worker unchanged. Hardware discovery, stability and OTA still need testing.

## Rust prototype allocation-safe SDK logging 0.1.3-rs-matter

The owner can now discover, assign and save four stations through the dashboard.
The restart and new login immediately after a successful configuration save are
intentional. A later 0.1.2 panic is separate: its exact published ELF identifies
failed UART mutex allocation in newlib, reached by Wi-Fi/coexistence logging.

Warm the shared UART write lock through public console write before Wi-Fi starts.
Keep the standard SDK formatter, levels and messages, including float/long-long
support. Add a numeric-only, allocation-free
heap-failure hook (requested bytes/capabilities/count), throttled to one report per
second, and a cumulative counter in periodic diagnostics. Keep the hook in IRAM
and its format/counters in DRAM, checked in the final ELF; GCC LTO requires a
non-inlined hook to preserve its section. No further stack/capacity reductions,
partition changes, credential resets or automatic power writes. Underlying
memory-pressure peaks and physical stability still require an owner retest.

## Rust prototype HTTP memory correction 0.1.2-rs-matter

The owner now reaches Matter/BLE ready on 0.1.1, but HTTP is unavailable with
about 2.4 KiB free data heap. Reduce the ESP32 future arena from 24 to 16 KiB,
checking the exact allocated future plus alignment and 1 KiB reserve at compile
time. Its frame is about 12 KiB on Xtensa; 64-bit host tests retain a larger arena.
Reduce the foreground task from 24 to 16 KiB using the owner's measured <=5964
bytes used during startup/idle. Pairing and OTA stack peaks still need hardware
checks. BLE stays at 8 KiB. This releases 16 KiB of fixed reservations while
retaining all station, fabric, session, exchange and subscription capacities.
Log arena used/capacity along with existing heap/stack diagnostics. Preserve the
cache-safe libc guard, partition table and NVS. HTTP/OTA and Alexa retests remain
required; software checks do not establish physical success.

## Rust prototype boot correction 0.1.1-rs-matter

Fix the 0.1.0 pre-dashboard cache-error reboot loop: link the Rust archive after
SDK libraries inside the link group, preserving ESP-IDF's cache-safe libc
implementations. Packaging validates all five affected final ELF symbols and
rejects weak, flash-resident or missing definitions. No partition or NVS changes.
Recover the boot-looping build using the four-part USB installer, erase unchecked.
Hardware startup and Alexa pairing still require an owner retest.

## Independent Rust prototype 0.1.0-rs-matter

Replace the experimental Tasmota backend, builder and installer with the native
Rust rs-matter implementation. The shared dashboard/API/authentication, sequential
BLE worker and validated OTA receiver remain. Preserve the complete partition
layout and retained station endpoint IDs. Independent protocol state uses NVS
namespace lhrsmatter; switching engines requires new pairing, without erasing
Wi-Fi, the own password, API key or station assignments. Stable and SDK Matter
releases are unchanged. Alexa compatibility and runtime memory need hardware tests.
