# Lighthouse Bridge — project instructions

Read this file before edits. **Update it in the same change** when requirements,
hardware, architecture, languages, auth, flashing, workflows or verification
status change. Keep current facts, not a conversation log. User instructions win.

## Purpose and hardware

- Repository `Traumlewis/Lighthouse-Bridge`, authorized working branch `main`.
- The owner requires an explicit question before future device changes,
  especially firmware writes, resets and new commissioning actions. Credentials
  do not grant standing mutation approval. The owner approved the 0.1.9,
  0.1.10, 0.1.11, 0.1.12, 0.1.13, 0.1.14, 0.1.15, 0.1.16, 0.1.17 and 0.1.18 OTA updates and the specific unbound-endpoint-1 aggregator exception; ask again
  before later updates or separate device changes. Read-only checks may proceed.
- The owner defers code cleanup until an explicit go-ahead such as
  "Aufräumen lohnt sich" is sent as an instruction. Discussing or quoting that
  future phrase is not authorization to start cleanup.
- The owner separately authorized the rs-matter Matter Setup UI simplification:
  load private status/active code on explicitly opening the section, remove the
  Show active code/status button, and keep only New code & open pairing window
  as a mutation action. The owner separately approved installing 0.1.10. This
  does not authorize the deferred general cleanup or a subsequent device change.
- Classic dual-core ESP-WROOM-32 / ESP-32S, CH340 USB-C, at least 4 MB flash.
  PlatformIO `esp32dev`. Never substitute another chip's firmware on this board.
  The owner additionally requests rs-matter profiles for WeAct ESP32-C3FH4,
  ESP32 C3 SuperMini and ESP32 S3 SuperMini, selectable in the WebFlasher.
  This supersedes the classic-only restriction for those separate rs builds;
  stable and SDK builds remain classic ESP32. Preserve both existing app slots
  and the complete partition table. Chip-specific USB boot offsets, Rust targets,
  OTA chip validation and installer manifests must agree. S3 owner specification is
  4 MB flash and 2 MB on-chip PSRAM; use the QSPI PSRAM profile.
  No new-board hardware validation or device mutation is authorized by this request.
- Installed **0.1.18-rs-matter** adds two optional active-low GPIO buttons,
  assigned through personal-session/Origin-protected settings to All on/All off.
  Default disabled, conservative pin allowlists exclude boot/flash/USB/UART pins;
  use INPUT_PULLUP and momentary contacts to GND. Require stable release before
  arming, debounce 40 ms, one action per press, no hold repeats or startup writes.
  Feed the existing command queue; OFF respects each station's saved preference.
  Persist both bindings atomically under lhbridge/gpio_buttons; invalid records
  disable buttons. Setup/OTA/restart gates reject commands without deferred replay.
  All four full builds, exact partitions/OTA/chip/hash/cache-off guards, 34 Python
  checks, 15 browser/USB programs and eight release Rust tests pass. Twenty native
  programs including GPIO/OTA chip variants pass. Final apps: classic 1,954,768,
  both C3 profiles 1,945,024, S3 1,880,016 bytes (slot 1,966,080). Static RAM:
  67,596 / 55,048 / 64,036 respectively; these are not runtime margins.
  BOOT is GPIO9 on C3, GPIO0 on ESP32/S3. Apply the host-only NimBLE config after
  SDK defaults in every C/C++ TU; legacy role aliases must stay undefined to
  avoid NimBLE's rename shim turning disabled roles into empty defines. C3 uses
  portable-atomic 1.15.0 with the existing critical-section implementation and
  foreground-only logger registration for its non-CAS target. No dependency
  versions were upgraded. Classic arena/Stack/task remain 14 KiB/52,528/18 KiB;
  stable and SDK builds pass. New GPIO wiring/C3/S3 runtime behavior is unverified.
  Owner-approved classic OTA from 0.1.17 succeeded with one upload. Pre/post
  comparisons retained four assignments/OFF choices, credentials, naming and
  network mode; the board reports home Wi-Fi connected, Matter ready/paired and
  usable BLE. Live EN/DE browser checks at 1280/390/320 confirm the new GPIO UI,
  both bindings disabled, no overflow/page errors or power/config/commissioning
  writes. Diagnostic sessions are closed. The initial verifier decoded gzip as
  plain text after successful settings checks; read-only browser verification
  resolved that diagnostic error without a second upload. The owner subsequently
  authorizes publishing 0.1.18 and the board selector through the existing Pages
  workflow; follow CI/deployment and verify live manifests/checksums.
  Physical button operation and new-board hardware behavior remain unverified.
  The stable installer board notice explicitly scopes classic-only support to
  Hue firmware and links C3/S3 owners to the separate rs-matter board selector.
  Public publication is verified: commit 38860c3, GitHub Actions run
  43707555895 completed build+deploy successfully. Live main-page notice and
  all four board selectors/downloads pass EN/DE 1280/390/320 browser checks,
  with no page errors or upload requests. Downloaded manifests, chip IDs,
  offsets, complete partitions, image/OTA checksums and ZIP payload/source
  match the released code. Public CI app sizes are 1,954,544 classic,
  1,944,800 both C3 profiles and 1,879,792 S3; local installed build sizes above
  differ with build paths. This is publication/software evidence, not new-board
  hardware testing. No separate device change is authorized.
  Ask before any later hardware update or separate device change.
- Up to **16 Valve Lighthouse V2**, controlled by **Alexa**. The owner currently
  has four physical stations; this is test evidence, not a capacity/model restriction.
- **ON = `0x01`; OFF means SLEEP = `0x00`. Never send STANDBY (`0x02`).**
  STANDBY may only appear as a measured state in stable/SDK firmware. The owner
  explicitly supersedes this rule for rs-matter 0.1.12: each station can opt into
  STANDBY (0x02) for OFF; defaults and existing records remain SLEEP.
- No extra phone companion app. General local HTTP API for integrations. Do not
  introduce smartwatch-specific plans. Local firmware upload is implemented in
  0.3.0; Improv Serial is not implemented.
- Never send power commands automatically on boot/reboot/Wi-Fi reconnect. Pure
  BLE reads at startup and periodically are intended and required.

## Room label and confirmed maintenance

- Candidate 0.1.19-rs-matter adds a private persistent room/owner label under
  lhbridge/room_name, independent of network naming, 1–64 valid UTF-8 bytes.
  Edit by clicking the heading; explicit Save/Cancel. Hide only the intro heading
  and paragraph when at least one actual station is configured. Default label
  remains localized; saved names render as text and survive language changes.
- rs-only Dangerzone shows setup SSID/current API key as copyable setup password,
  with plain text button styling, Restart and explicitly confirmed Factory reset.
  Personal-password session and Origin/JSON gates protect POST /api/room and
  /api/maintenance; API keys/bootstrap sessions cannot authorize either.
- Restart preserves settings. Full reset durably marks lhbridge/factory_reset,
  pauses new work and waits at least 1.5 s plus physical BLE idle before reboot.
  Reject scans/commands/setup/OTA. At the next boot, before Preferences/WiFi/BLE/
  Rust initialization, erase only standard NVS (all Wi-Fi/auth/API key, room,
  stations/endpoint registry, buttons and Matter fabrics). Never erase app slots,
  OTA selection or the partition table. Failure stops startup with a redacted
  diagnostic. Do not claim power-loss/real-device reset verification.
- After reset, retrieve the new generated setup Wi-Fi password via physical USB
  and repeat initial setup/Matter pairing. Clear the obsolete key in the browser
  after reset acceptance. Restart/reset and code publication are separate from
  device-install approval; no 0.1.19 device mutation is authorized.

- 0.1.19 local builds fit all unchanged slots: classic 1,959,440, both C3
  1,949,312, S3 1,884,976 bytes; static RAM 67,620 / 55,072 / 64,052.
  Cache-off guards, stable/SDK regressions, Pages, 34 Python checks and all
  16 browser/USB programs pass (mocked HTTP/USB).
  All native CI programs (including GPIO/OTA chip variants and the new
  maintenance program) and eight Rust tests in both debug/release pass.
  These are software checks, not device reset or runtime memory evidence.

## Status and BLE

- `src/main.cpp` owns a single BLE worker: scans, connects, reads, writes and
  disconnects sequentially. HTTP/SSDP must stay responsive.
- Service `00001523-1212-efde-1523-785feabcd124`, power characteristic
  `00001525-1212-efde-1523-785feabcd124`. Three command attempts maximum.
- New queued commands replace older pending commands for the same station.
  Old results must not clear newer requests. Identical pending/busy goals are
  accepted without replacing the job, bumping its generation or acceptance time;
  a new target supersedes it, and requests after completion can retry normally.
  Always connect with **Random (1)**; no address-type UI. Reject duplicate station MACs/names.
- `requested` is only a goal. **Dashboard and local API always use `observed`
  measurements**, with timestamps/reachability. The owner explicitly approved a
  Hue-only target window after the 0.2.5 Echo retest: while an accepted command
  is pending/busy, Hue `on` follows the target for at most 30000 ms from acceptance.
  Completion (including unconfirmed writes), final error or expiry restores the
  measured `on` immediately. Never extend the window on duplicate active requests.
  `reachable` stays measured throughout; do not force it true or modify observed
  values/timestamps. No window exists after reboot or without a command.
- Keep `command_sent` separate from `command_confirmed`. After a successful
  power write, only observed steady ON (11) confirms ON, and only SLEEP (0)
  confirms SLEEP; booting 1/8/9 and STANDBY (2) do not confirm those targets.
  If the immediate read does not confirm, use read-only checks about every 2 s
  for at most 30000 ms from last_sent_ms. These yield to queued commands and use
  existing bounded read attempts; never add automatic power resends.
- Last command status is none/queued/sending/failed/verifying/confirmed/unconfirmed.
  A confirmation latches for that command; later external state changes do not
  rewrite its history. New accepted work resets it. Matching observations outside
  the verification deadline update measured state but do not confirm that old
  command. Failed writes do not open a verification window. Old-generation reads
  cannot confirm a newer target. `readInterval(state, now)` needs current millis.
  The UI labels this as last-command history beside current measured state.
  Verification never extends the separate owner-approved Hue target window.
- Capture status uptime/age/health under the same state lock so a newly recorded
  measurement cannot have a timestamp newer than the returned uptime snapshot.
- Read on startup, after commands, and about every 15 seconds; booting states
  retry after 2 seconds, unreachable devices after 30 seconds. One radio means
  queue/connection delays can lengthen intervals. Manual refresh is read-only.
- Pin NimBLE-Arduino **2.5.2** (Arduino ESP32 remains 2.0.17). Connection and
  scan durations use **milliseconds** in the 2.x API: `setConnectTimeout(3000)`,
  blocking `getResults(2000, false)` for recovery, `getResults(5000, false)` for
  discovery in stable firmware. Matter 0.3.6 uses the same durations with raw
  legacy GAP scanning (see below), avoiding result-vector copies. Stable results
  contain const device pointers; copy data before clearing.
  `deleteClient()` can be asynchronous in 2.x: never touch the pointer afterward.
  Wait for created-client count zero before any further scan/connection; keep
  bleActive true during deferred cleanup to block the Wi-Fi portal. If cleanup
  exceeds 5 seconds log once and keep the worker waiting (HTTP remains serviced).
  Do not stamp a successful read later just because cleanup takes time.
- Set `setConnectRetries(1)` explicitly: at most one internal retry of HCI 0x3e
  per client connection call. Read-only queries use up to two worker attempts,
  with 350–650 ms between attempts. Yield to queued commands/reassignments before
  and after retry scanning. Each controller connection attempt has a 3-second
  timeout. At most four controller starts per read job, six per command job,
  with at most three write attempts. Do not silently multiply library defaults
  and worker retries. Allow 150 ms between jobs. Internal retry/control-protocol
  delays and GATT work can lengthen a job; do not promise a hard total deadline.
- BLE code 574 = 0x023e = HCI 0x3e, connection establishment failed. On this
  error only, scan passively for 2 seconds before the next attempt. Match the
  assigned MAC, require connectable advertising and log its observed address type;
  connections remain Random (1). Do not persist changes or accept a device by name alone.
  If unseen, retry the saved address anyway (a short scan is not proof of absence).
  Log the actual phase/code/attempt and recovery RSSI/type or lack of advertising.
  Recovery scans never write power and do not replace the user discovery results.
- On classic ESP32, classify legacy advertising using
  `isConnectableLegacyAdvertisement(getAdvType())`: only ADV_IND (0) and
  ADV_DIRECT_IND (1). **Do not use NimBLE 1.4.3 `isConnectable()`**: its
  non-extended path treats enum values as bit flags, rejects 0 and accepts 3/4.
  Retain explicit legacy classification on 2.5.2. Firmware static assertions
  bind enum values to upstream HCI constants; native tests cover all five report types and an invalid value. Log a matched but
  non-connectable report separately from no matching address, with type/RSSI.
- One failed read cycle retains a previously usable measurement, explicitly labeled
  last reading / retrying, without changing its timestamp. Retry after 5 seconds.
  Two consecutive failed cycles, no prior reading, or age >=60 seconds means
  unreachable. Counters saturate and reset on a successful reading. Never grant
  this grace to a write with no post-write measurement: that starts unreachable,
  regardless of the older cache. Verification polling takes precedence over normal failure backoff while its read-only window is open. Expose read_failures, ble_error
  and last_read_attempt_ms in status; log numeric BLE codes without device keys.
  Log successful reads too, including worker attempt and raw state; error logs
  include saved/used address type and the explicit host retry limit.
- Reassignment may proceed during a background read. Check its generation before
  applying the result; never apply an old device reading to a new assignment.
  A separate physical BLE-active flag must still block starting the Wi-Fi portal.
- Preserve API `reading` as active OR queued for compatibility, with separate
  `read_active` and `read_queued`. UI must distinguish those phases and label age
  as the last successful measurement's age, not an ongoing read's duration.
  Recent unusable measurements say unavailable; only age >=60 s says stale.
- Readings older than 60 seconds are stale. Raw 1/8/9 = booting, 11 = ON, 0 = SLEEP, 2 = STANDBY.
  Booting counts active for Hue; dashboard still says Starting.
- Alexa exposes only individual entries with an ID and nonempty MAC. No virtual
  group light; Hue identity 5 is permanently retired (local/API ID 5 is valid). Cached VR Basisstationen must
  be deleted once by the owner in Alexa; the bridge cannot delete cloud entries.
- Hue discovery serialization lives in `include/hue_api.h` and is host-tested.
  Hue identities 1–255 use the pinned Espalexa zero-padded hexadecimal endpoint format
  `bridge-MAC-%02X-00:11`, including newly added IDs 6–9. Do not use unpadded
  decimal IDs: 0.3.0/0.3.1 introduced that regression for IDs >4. Larger IDs use
  a fixed EUI-64 + endpoint representation (local prefix, existing 24-bit bridge
  identity, full station ID); never truncate the monotonic ID or reuse identities.
  Preserve old Alexa identities and numeric Hue keys. Test actual serialized collection
  fields and string lifetimes as well as PUT acknowledgements.
- Dynamic list: 0–16 entries. UI supports explicit Add/Remove and one MAC input
  with discovered datalist suggestions. Empty added entries are disabled and
  count toward capacity. Legacy empty slots are discarded during migration.
- Local/API station IDs are fixed slots **1..16**, clarified by the owner after
  0.3.6. New entries get the lowest free slot; deleting/reordering never renumbers
  survivors. Removed slots can be reused. Reserve retained IDs before allocating
  new entries, regardless of payload order. Store/display entries sorted by ID.
  Card labels, form headers/default names, API and logs use the actual fixed ID.
  Pending unsaved entries preview the same lowest-free allocation.
- `StationConfig.hueId` is a separate stable monotonic Alexa identity. Skip Hue 5
  permanently and never reuse removed Hue identities. Pre-0.3.6 dynamic IDs 6–9 migrate
  to local 1–4 while preserving those Hue identities and encoded keys/unique IDs.
  Legacy four-slot records preserve original Hue identities; discard empty slots.
- NVS atomically stores {next_id,stations:[{id,hue_id,name,mac}]}; next_id is the
  next Hue identity, retaining the existing record key. Public config omits
  hue_id; clients cannot replace it. Existing request IDs refer to pre-save local
  IDs, new entries use 0, and the response returns assigned fixed-slot IDs.
  Browser power controls stay disabled during save until a fresh status is loaded;
  ignore older in-flight status responses. Integrations must refresh after removal/replacement: a freed slot may later
  refer to a new device. Do not interpret array positions as IDs. Hue PUT/GET translate via the stable identity.
- Config saves may run during a background read but reject pending/active commands
  or scans. State/history follows unchanged Hue identity+MAC across reordering; generation
  at each destination index increments to invalidate old in-flight readings.
- `include/hue_api.h` owns shared Hue light-route parsing and write response
  generation, tested against the same ArduinoJson dependency on the host. Strict
  `PUT /api/<username>/lights/<actual-id>/state` parses boolean `on` only and
  queues ON/SLEEP immediately. Success is HTTP 200 with
  `[{"success":{"/lights/<actual-id>/state/on":true|false}}]`, with the accepted
  target value. Never fall back to Espalexa's hard-coded light-1 ACK for these
  routes. Errors use Hue error arrays; malformed IDs/JSON/values never enqueue.
  Unsupported properties receive error 6 without triggering power by themselves;
  valid `on` can be accepted independently. Bound bodies to 512 bytes/8 properties.
- PUT acceptance does not imply the later BLE operation succeeded. Apply only
  the owner-approved, bounded Hue `on` window above. Keep measurement data and
  reachability unchanged; no permanent optimistic state or forced reachability.
  Local API exposes requested_ms and alexa_target_pending without altering its
  measured observed/observed_raw fields.
  Preserve existing individual IDs and names; Espalexa still handles SSDP,
  description and username registration, not light PUTs or measured light GETs.
- Serial `Hue PUT` / `Hue GET` diagnostics include the current local station ID, target or
  measured value, acceptance/reachability and pending work. Collection GETs log
  configured/fresh counts. Log no usernames, raw URLs, bodies or access keys.
  GET source=accepted_target or source=measurement identifies the selected Hue
  view (collections report whether any accepted target contributed). PUT handler_ms
  measures the handler only, not network/end-to-end latency.
- Since 0.3.8 Hue GET includes read-only `bri` 0/254 mirroring the returned `on`,
  plus `mode:homeautomation`. This is an experimental compatibility hint, not
  physical brightness or a dimming capability. Standard profile remains On/off light / HASS321; preserve all identities.
  Optional compatibility behavior is described below. Reachability/timestamps
  remain measured. The hint shares `on`'s accepted-target window exactly.
- 0.3.9 adds per-station `alexaCompat` / public+NVS `alexa_compat` boolean, off by
  default. True advertises Dimmable light / LWB010, without changing either ID.
  In this profile only, Hue accepts integer bri 0..254 as binary 0=SLEEP/nonzero=ON.
  Explicit valid boolean on overrides bri. Queue once per request and acknowledge
  the effective 0/254 binary result; never claim physical dimming. Standard
  brightness-only writes stay error6. Invalid fields cannot send power by themselves.
- Preserve the profile on existing entries when config input omits it, default
  false for old records/new stations. Reject non-boolean values. UI translations,
  unsaved edits and reload must preserve the checkbox. Other stations stay standard.
- Profile changes may need removing/re-discovering just that Alexa device to refresh
  capabilities; explain that groups/routines may need restoring. Start with one
  station, not a mass reset. No actual Alexa hardware is attached for validation.
- Owner's 0.3.7 sample proves fresh GET with on=false/reachable=true can still
  leave the Alexa app showing ON; 0.3.8 plus app-cache clearing did not help. Do not claim missing polling/push or caching is
  the established cause; full JSON was valid. Additional fields are a hypothesis,
  not a verified app fix. No physical Echo/ESP32 is attached to this environment.
  Never commit the raw response, which contains the owner's hardware MAC.
- **Local Hue emulation cannot push status into the Alexa app.** Alexa controls
  its own polling/cache. Do not promise immediate app-state synchronization.
- Owner's discovery/setup retest shows all four individual stations labeled Offline
  in Alexa's naming screen. At 21:59:10 the supplied log has two GETs per station,
  all profile=standard/on=false/bri=0/reachable=true with no pending command.
  Later, station 1 legitimately becomes unreachable after exhausted BLE read
  cycles; stations 2–4 still return reachable=true. These failures do not explain
  all four Offline labels. No PUT appears in this excerpt. Exact firmware version
  and screenshot-to-log timing are not captured; the optional compatibility profile
  remains untested in this evidence. Owner recalls status displaying in 0.2.6;
  a controlled old/current comparison and the regression cause remain open.
- A subsequent station-1 compatibility-profile retest still reports Alexa
  "Device is unresponsive". At 22:10:39, 22:10:52, 22:11:22–23 and 22:13:24
  its GET diagnostics show profile=compat/on=false/bri=0/reachable=true,
  pending=false/source=measurement; collections at 22:11:22 have fresh=4.
  Station 1 has repeated successful SLEEP reads. No Hue PUT is shown.
  This is negative owner evidence for the profile experiment, not a confirmed
  fix; handler logs do not prove client receipt/parsing or capability refresh.
- Keep Wi-Fi modem sleep enabled (`WIFI_PS_MIN_MODEM`) before BLE starts and
  after reconnect. Set Arduino's cached policy before STA starts, and check the
  driver result. `WiFi.setSleep(false)` / `WIFI_PS_NONE` caused the 0.1.0 startup
  abort in `coex_enable()`. This setting is unrelated to Lighthouse SLEEP.

## Wi-Fi, passwords and USB

- WiFiManager uses a **non-blocking** protected setup portal so USB info can be
  serviced before Wi-Fi is configured. Normal HTTP starts after connection and
  BLE initialization. BOOT held for 5 seconds opens setup when BLE is idle.
- Background reads must not flicker/disable the Wi-Fi setup button. A portal
  request pauses new BLE work and waits for physical idle. Commands/scans block it.
- Manual portal pauses BLE/normal HTTP and reboots after save or 3 minutes.
  Initial portal has no timeout. Persist credentials to ESP32 FLASH/NVS.
- API key = device-generated 24 hex characters, stored under `access_key` in the
  `lhbridge` NVS namespace. It remains the setup-AP password and integration key.
- **First web login** uses `admin` + API key, then forces an own web password.
  Bootstrap sessions cannot use the dashboard before password setup.
- Later web logins use the own password, **not the API key**. Store only PBKDF2-
  SHA256 hash + random salt (10000 iterations) as one `web_auth` NVS record.
- HttpOnly, SameSite=Strict sessions (up to 12 hours, lost on reboot); password
  changes invalidate older sessions. Five failed logins cause a 30-second pause.
- Integrations use `X-API-Key`, independent of browser sessions. It must not grant
  access to `/api/account`, `/api/password`, `/api/device` or `/api/ota*`. Those require a web session.
- After own-password setup the current API key **must remain visible** to the
  signed-in user under **Wi-Fi & local API / WLAN & lokale API**. Never expose it
  in public HTML or general status, and never expose the own password/hash/salt.
- `LH_INFO\n` over physical USB at 115200 baud returns `LH_INFO {protocol:1,…}`
  with key, setup SSID, Wi-Fi connection, URL/null, password-set flag and version.
  Serial commands remain serviced while the setup portal is active.
- `LH_RESET_WEB_PASSWORD\n` is physical-USB recovery: clears only web auth and
  sessions, leaving key, Wi-Fi and station assignments intact. Never auto-send it.
- `site/serial-info.mjs` reads USB after the installer closes, or an explicit
  Read key & address click. One serial owner at a time; disconnect before flashing.
  Query every 2 seconds, allow 45 seconds for first response. Validate frames and
  IPv4 HTTP links; keys stay in page memory, not storage, URLs or network calls.
- Hue endpoints intentionally need no login for the trusted local Echo. HTTP/NVS
  are unencrypted; do not enable public network exposure. Never commit real keys,
  passwords, SSIDs, device MACs, or raw serial logs containing them.

## Dashboard debug details

- Normal view hides detailed read/retry errors, background-read phases, measurement
  ages, last-reading suffix and last-command history. Preserve measured badges,
  unavailable/first-read status and queued/sending power targets. Owner reverted
  the warning-only 0.3.4 behavior, with an explicit requirement for no layout jumps.
- Reserve a fixed four-line status area in every card even when text is empty.
  Long debug text scrolls inside it (keyboard focus available in debug). Keep badge
  height fixed and reserve the footer indicator's space using visibility rather
  than display:none. Toggling must not move cards, buttons or footer on EN/DE
  desktop/mobile layouts. Do not fabricate fresh measurements.
- Three version-button activations within 1200 ms toggle debug; another triple
  disables it. Keep native keyboard support and EN/DE indicator/accessible labels.
  Single/double/slow unrelated clicks must not toggle it. No network mutations.
- Default off on page load and logout, RAM only; language changes preserve it.
  Debug affects browser presentation only, never BLE scheduling, serial or API.

## Device naming, copy and OTA

- Click-to-copy API key in the USB WebFlasher panel and authenticated dashboard.
  The owner wants both WebFlasher and dashboard keys to look like plain clickable text: no visible
  button background, border or padding. Keep native button semantics/keyboard
  support but override generic button styling; accent-on-accent made it unreadable.
  Check contrast against the panel with a visible key, not only before connection.
  Support local HTTP with selection/execCommand fallback; no key storage/network.
- Custom device name: lowercase DNS label 1–32 bytes, digits/internal hyphens.
  Same saved name for setup AP SSID and mDNS hostname. Default legacy names stay
  until customized; do not alter saved Wi-Fi credentials. Portal styling matches
  dashboard palette. Hostname route is personal-password-session only.
- WiFiManager raw-BIN routes /update and /u are blocked with first-match handlers
  in its webserver callback (pinned library registers vendor routes afterwards).
  Do not leave a second, unvalidated firmware-upload path in the setup portal.
- OTA writes only the inactive app slot via Arduino Update; unchanged partition
  table. Personal-password session + session-bound prepare token + matching
  Origin (if supplied), no API-key/bootstrap access. Pause BLE; wait physical idle.
- Official .lhota = 64-byte header + raw app. Magic zero-padded to24, LEuint32 size
  at24, SHA256 at28, zeros60..63; classicESP32 chip/appdesc and embedded product
  marker required. Reject corruption/truncation/trailing data/multiple files.
  Branding/digest is not a digital signature; do not claim signing or crash rollback.
- Begin writing only after prefix validation; select boot slot with Update.end
  only after complete authorized multipart request and length/product/digest check.
  Abort on disconnect/failure; previous boot selection remains. Idle preparations
  expire60s. Success responds then reboots after1.5s. Settings remain in NVS.
- Initial USB upgrade to >=0.3.0 is needed for the OTA UI. Future .lhota packages
  are downloadable from Pages and ZIP/checksums, separate from the four USB parts.
  Do not accept merged factory BINs as update packages.
- Hardware OTA/power-loss/16-station operation remains unverified. Software tests
  and simulated browser uploads must never be described as actual device flashes.

## Critical flash-layout rule

- Experimental **0.3.9-matter** is a separate, multi-station Matter prototype in
  `experimental/matter/MatterBridge/`, offered at the existing Pages `matter.html`.
  Stable firmware stays **0.3.9** with its existing PlatformIO environment. The
  owner explicitly rejected larger/single app slots: **retain both 1.875 MiB slots
  and the exact complete partition table for the prototype too**. NimBLE makes the
  size feasible; the minimal Bluedroid probes exceeded the slot. Never package an
  oversized build or substitute S2/S3/C3 firmware.
- The isolated Arduino CLI build pins CLI 1.6.0, Arduino ESP32 3.3.12,
  NimBLE-Arduino 2.5.2, ArduinoJson 6.21.5 and WiFiManager 2.0.17. CHIPoBLE is compile-time disabled;
  commissioning is on-network after saved Wi-Fi connects. The worker alone owns
  NimBLE and uses shared ON/SLEEP/measurement/coalescing/confirmation logic.
  Matter command-handler interception queues commands without updating OnOff
  optimistically; measured OnOff and bridged Reachable updates run on the Matter
  task. No automatic power commands, extra phone app or raw credential logs.
- The prototype advertises separate bridged **On/Off Light** endpoints.
  Version 0.1.0-matter deliberately used an On/Off Plug-in Unit, which Alexa
  classified as an outlet. The owner reports basic operation works on hardware;
  specific command confirmations, external status synchronization and long-term
  reliability are not established by that report. The owner subsequently confirms
  0.1.1 ON/OFF, light classification and status display. This is a single-station
  owner test, not multi-station/long-run or specifically external-state evidence.
  Preserve endpoint/unique IDs and commissioning credentials;
  Alexa may need removal/re-pairing of only the prototype device while connected
  to refresh its type. Groups/routines may need restoring. Never erase NVS for this.
- Experimental scope is 0–16 configured stations with nonempty MACs, sharing
  the production dashboard, authentication, local API, discovery/editor and OTA
  receiver. Stable settings are retained. Station saves checkpoint identities,
  persist configuration, pause BLE and restart after 1.5 seconds; reject power
  while restarting. No automatic power command is sent. Install/return by USB
  with erasing unchecked; Hue entries are unavailable while Matter runs.
  Per-board setup credentials and endpoint registry live under `lhmatter`;
  Matter also persists fabric data. Test bridge name is exactly
  **Lighthouse-Bridge Matter Test**, later **Lighthouse-Bridge** after testing.
  Never rename saved station names.
- `include/matter_config.h` owns the persistent endpoint registry under
  `lhmatter/endpoints`. Bind by full stable Hue identity + MAC, not local slot or
  array position. Migrate the former first station to endpoint 1 and retain
  aggregator 2; allocate other endpoint IDs monotonically from 3, never reuse a
  retired endpoint. Validate the registry; fail closed on corruption/exhaustion. Invalid saved
  station records must not prune/rewrite Matter identities during boot.
  Checkpoint current+proposed bindings (up to 32) so interrupted saves cannot
  renumber survivors; current active capacity remains 16. No silent truncation.
- The Matter sketch includes production `src/main.cpp` with `LH_MATTER_BUILD`;
  `matter_adapter.h` replaces Hue, never the sequential BLE worker/auth/OTA.
  `tools/build_matter.py` uses Python 3.12 plus pinned Zopfli (requirements file)
  to gzip-compress the shared embedded UI, sets C++ only
  `-Oz -flto` and C `-Oz`, links with LTO and disables WiFiManager debug output.
  Do not enable C LTO: the pinned SDK backtrace weak symbol fails to link.
  Additional registry/lamp objects use checked heap allocation. Static linker
  size does not prove runtime memory headroom. Keep both app slots unchanged.
- Owner hardware evidence for full 0.3.0-matter is negative: the HTML loads,
  status reports OUT_OF_MEMORY, discovery has no visible result/log and the
  Matter panel is absent. Do not describe the full version as working from the
  earlier single-light test. Version 0.3.1-matter removes the fixed 32-KiB status
  document and complete response String. `include/status_api.h` owns a coherent,
  bounded (<6 KiB) heap snapshot and the host-tested 256-byte streaming serializer;
  each JSON item uses bounded stack storage. Never hold stateMutex during network
  IO, truncate stations/results or split uptime from measurement capture.
  `matter_status.h` streams the production route using that helper. Small Matter
  JSON replies also stream without a full-body String. Config and registry JSON
  capacities derive from all 16 stations/32 transition bindings, tested with
  maximum names/MACs and serialized restoration. Preserve all existing fields.
- Personal-session `/api/account` identifies Matter and readiness without a code,
  so the setup-code panel remains reachable if general status fails. Bootstrap
  sessions still cannot show the panel or retrieve the code. Scan queue/start/end
  logs are read-only and contain no device identifiers. Startup/status expose
  free heap/largest block/minimum for diagnosis, never claim hardware RAM margins.
- Owner's 0.3.1 private-info response confirms ready=false/paired=false,
  error=MATTER_BRIDGE_FAILED and pairing_code=null; the UI only showed the bridge
  name. Version 0.3.2 starts the root Matter stack before scheduling bridge/light
  construction on the CHIP task. Never resume endpoints before Matter.begin():
  SDK resume requires an already allocated ID below its restored counter.
  `matter_endpoint.h` selects resume/create with the restored counter and preserves
  exact IDs, skipping gaps using empty destroyable endpoints that are never enabled.
  Read SDK counter metadata only (pinned namespace/key, legacy fallback); never
  edit SDK keys/fabrics or use private struct casts. Only SDK create advances it.
  Enable completed endpoints and invalidate root/aggregator PartsList. Block config
  saves during initialization and keep commands rejected until all endpoints pass
  handler validation. Startup failure remains explicit with a non-secret phase.
- The setup-code button owns its text directly, without a nested code-node lookup;
  display status/errors before the code. Copy the code from authenticated response
  state, not translated DOM text. Preserve language/logout/privacy behavior.
  The missing-descendant renderer failure is reproduced in software, not proven
  as the cause of the owner's browser behavior. Do not claim physical startup
  success from simulated allocation or browser tests.
- Owner's 0.3.2 boot reaches Matter endpoints ready and BLE ready, then fails
  to create the BLE worker and never starts HTTP. The supplied log contains real
  credentials: never copy it, its key or device identifiers into source/docs/tests.
  Version 0.3.3 reserves the unchanged 8192-byte worker task before Matter heap
  allocations. The task waits on an atomic gate until BLE initialization succeeds;
  it cannot scan/connect/read/write early. `startup_sequence.h` owns the tested
  reserve -> HTTP -> BLE initialization -> Matter completion -> release order. Free the boot-config text
  before reservation. Do not shrink stacks without measured high-water evidence.
- Worker reservation, BLE init or modem-sleep failure must not trap Matter firmware
  before HTTP. Preserve authenticated web/OTA/USB recovery, reject radio requests
  with 503 and the startup error, disable radio buttons but keep firmware upload
  enabled. App/Matter readiness/code/window require a usable worker as well as
  completed endpoints. Expose `ble_worker_available` / `ble_startup_error` and
  non-secret reservation/free/largest-block diagnostics. No automatic power writes.
  If the Matter stack never started, HTTP mDNS may start without taking over an
  active Matter DNS-SD service. Recover an already stalled 0.3.2 via four-part USB
  with erasing unchecked; its HTTP OTA service was never reached.
- The authenticated dashboard retrieves the manual code on explicit GET
  `/api/matter`; a personal-password web session is required, never API key or
  bootstrap. General status contains no pairing code. POST
  `/api/matter/commissioning` uses the same session and Origin/JSON checks and
  schedules a 15-minute Basic Commissioning Window with DNS-SD only on the CHIP
  task. Existing open/pending windows coalesce; no automatic reset or power writes.
  Codes stay in page memory, clear on logout, and are not in URLs/storage.
  Matter owns mDNS: set its hostname/add HTTP service without MDNS.end().
- `tools/build_matter.py` writes split images, `.lhota`, checksums and source ZIP
  to `dist/matter/`. Pages validates exact partition-table equality, slot/chip/hash
  checks and that the OTA payload is identical to the app. No merged Matter image.
  USB uses `LH_INFO` plus `LH_MATTER_INFO` through one serial owner: the former
  returns setup key/address, the latter measured stations/code, never a MAC/key.
  Retain validated parsers, EN/DE edits/connections and experimental consent.
- No hardware is attached to the development environment. The owner's initial
  0.1.0 result supports basic operation; runtime memory, persistence/return across
  framework versions, prolonged BLE coexistence and exact Alexa status behavior
  remain unverified. The 0.1.1 lamp/switch/status report is positive owner evidence;
  multi-station and full 0.3.3-matter still need hardware testing.
  Test attestation is not product certification. Do not claim a fix from a build
  or simulated browser test. Keep the prototype guide and validation report current.
- The owner now requests the full dashboard/API/OTA integration and web-accessible
  Matter setup code without waiting for the multi-station hardware test.
  The single-station 0.1.1 test is owner-confirmed. Full 0.3.3-matter behavior,
  actual OTA, runtime memory and multi-station operation remain hardware checks.

- Current release **0.3.9**. USB WebFlasher updates use exactly four files at offsets:
  bootloader `0x1000`, partitions `0x8000`, boot-app0 `0xe000`, application `0x10000`.
- **Never use a merged-at-zero image for a settings-preserving update.** Its
  padding overwrites NVS even with eraseAll=false: this caused 0.1.x credential
  loss. Keep `0x9000..0xdfff` untouched, including sector-erase boundaries.
- The merged BIN at `0x0` is a **factory-only** download, clearly labeled as
  resetting settings. Keep the erase choice and explain to deselect it for updates.
- Image URLs carry their SHA-256 query, so the installer's normal browser cache
  cannot reuse older firmware after a rebuild. Keep version tags on page assets
  current when changing the public app/copy to avoid mixed cached UI versions.
- Do not change NVS/partition locations casually. Current two 1.875-MiB app slots
  and NVS remain unchanged; no filesystem upload is needed for the embedded UI.

## Version history

- Keep release notes in the root **VERSION.md**, newest release first, in English.
  Add/update its entry when releasing firmware. README contains current features,
  setup/use instructions and a link to VERSION.md, not a chronological changelog.
- Documentation-only changes do not bump the firmware version. Keep the current
  release in VERSION.md aligned with firmware/package metadata for real releases.
- Package VERSION.md next to READ-ME-FIRST.md in release ZIPs as well as in source/.

## Languages and source layout

- **Keep the GitHub repository in English:** README, project documentation,
  authored comments, non-localized diagnostics, commit messages and generated
  release instructions. Do not add German-only repository documentation.
  German belongs in intentional UI translations and tests of those translations.
  Preserve upstream licenses; do not rename user/device data for language consistency.
- Speak German to the owner in chat unless asked otherwise; repository language
  and conversation language are separate preferences.
- **Both public WebFlasher and embedded dashboard default to English**, German
  selectable. Valid `?lang=en/de` overrides `localStorage['lighthouse-bridge.language']`,
  otherwise default English. Never auto-select German from browser locale.
- Translate statuses/errors, forms, help, metadata and accessible labels. Switching
  language must preserve form edits, selections, checkboxes and USB connections.
  Saved user/device names are not UI strings and must not be translated/renamed.
- Public copy: `site/translations.js`. Embedded copy/HTML/CSS/JS:
  `include/web_ui.h` (self-contained PROGMEM HTML). Upstream installer/OS picker
  keep their own language; do not modify vendored modules to translate them.
- `src/main.cpp`: setup, auth/routes, shared Hue state, worker and USB protocol.
  `include/bridge_core.h`: pure request/measurement logic.
  `station_config.h`: shared migration/validation/reconciliation. `ota_format.h`: update identity/header checks. `ota_receiver.h` owns the
  production streamed validation pipeline; test with fake IO and real package checks.
  `include/password_hash.h`: password derivation and random/constant-time helpers.
- `site/`: Pages source. `flash-layout.mjs` validates update manifests;
  `serial-info.mjs` owns USB parsing/reading. All URLs work below `/Lighthouse-Bridge/`.
- `site/vendor/`: unchanged ESP Web Tools 10.4.0, complete lazy imports and integrity
  hashes. No runtime CDN. `tools/package_release.py`: factory + split images/ZIP.
  `tools/build_pages.py`: vendor/integrity/layout checks and `_site/` generation.
- Version appears in firmware/status/USB, embedded footer and packaging script.
  Update them and docs together; browser tests read the generated manifest version.

## Validation and publication

```sh
pio run
g++ -std=c++11 -Wall -Wextra -Werror -I include -I .pio/libdeps/esp32dev/ArduinoJson/src test/test_hue.cpp -o /tmp/lh-hue-test
/tmp/lh-hue-test
g++ -std=c++11 -Wall -Wextra -Werror -I include test/test_core.cpp -o /tmp/lh-test
/tmp/lh-test
python tools/package_release.py
g++ -std=c++11 -Wall -Wextra -Werror -I include -I .pio/libdeps/esp32dev/ArduinoJson/src test/test_matter_config.cpp -o /tmp/lh-matter-config-test
/tmp/lh-matter-config-test
python -m pip install -r tools/requirements-matter-build.txt
python tools/build_matter.py
python3.12 -m venv .tools/tasmota-venv
.tools/tasmota-venv/bin/pip install -r tools/requirements-tasmota-build.txt
.tools/tasmota-venv/bin/python tools/build_tasmota.py
python tools/build_pages.py
python -m unittest discover -s test -p 'test_*.py' -v
node test/serial_info.cjs
node test/matter_info.cjs
npm test
```

Use `PLATFORMIO_CORE_DIR` / packaging `--pio-core-dir` for custom installations,
`CHROMIUM_PATH` for browser executable. Browser tests use mocked HTTP/USB, never
flash real hardware for rendering tests. Native tests cover request-vs-measurement,
read failure/staleness and millis wrap. Python tests simulate sector erase/write
with saved NVS and reject unsafe layouts. USB tests use actual parser/stream code.
For authentication/status/USB/language changes run the relevant browser tests.
CI builds firmware and runs native, Python asset/NVS and Node USB/layout tests;
ELF/map symbols are retained as `lighthouse-bridge-debug`. Keep generated `.pio/`,
`dist/`, `_site/` and `node_modules/` out of Git.

Owner confirms 0.1.1 startup/basic operation; the merged-image NVS overwrite
was corrected in 0.2.0. Persistent 574 failures also occur without another
controller, both ON/SLEEP, at about 1.60 m. Saved Random (1) types were already
correct; Public (0) produced timeout 13. The 0.2.3 scan bug was reproduced and
fixed, but visibility did not imply a successful BLE connection.
The owner reports 0.2.4 is more reliable for reads and group switching, but still
not perfect, and Alexa announces a problem even when switching works. The log
has 69 successful read jobs, 38 failed worker attempts (reads and commands),
24 sent command reports and one failed SLEEP command for station 4; it remained
ON in later reads until another command. These are log counts, not a controlled
failure-rate comparison. The log contains device credentials: never commit it.
The 0.2.5 retest confirms the warning remains: accepted PUT handlers take 6–16 ms,
then Echo immediately GETs the old opposite `on`, with reachable/pending true,
before BLE completes seconds later. OFF also addresses the group plus all four
individual lights. The owner chose temporary accepted targets for Alexa while
keeping dashboard/API measurements real. Version 0.2.6 implements that bounded
view and coalesces identical in-flight requests. The owner subsequently reports
that Alexa is happier (improved response, not a full validation claim). A later
0.2.6 excerpt shows source=accepted_target during commands, then measurement;
all shown ON/SLEEP actions are followed by raw 11/0, with intermittent 574 read
failures and recoveries. Remaining connection reliability is still open.
Version 0.2.7 adds explicit bounded read-only command confirmation; it does not
claim to fix the physical cause of 574 or alter BLE connection/retry parameters.
The owner's first 0.2.7 log (about 3.5 minutes) confirms all eight shown power
jobs: four ON targets reach raw 11 about 15–16 seconds after their sent reports,
and four SLEEP writes read raw 0 and confirmed=true immediately. It contains
51 successful read reports and five failed worker-attempt reports (including
read recovery and an interrupted read before a new SLEEP command). Do not count
these as five failed commands or derive a physical-link failure rate: internal
host retries are not logged individually. No shown power job fails. The new
confirmation behavior is now observed on hardware; long-run reliability and
other edge cases remain unverified. A subsequent approximately eight-minute
SLEEP-only excerpt contains 112 successful read reports (96 first worker attempt,
16 second), one exhausted station-2 cycle that recovers 9 seconds later, and no
power commands. Largest successful-reading gaps are 20/27/23/22 s for slots 1–4;
all eight logged Hue GETs report measured SLEEP and reachable=true. This supports
idle recovery/freshness behavior, not a new switching test or a physical-link
success-rate claim. Version 0.3.0 preserves its BLE timing/confirmation behavior.
Keep `docs/VALIDATION.md` current and distinguish simulated, CI and real evidence.

Inspect remote head/instructions and preserve owner edits/license; never force
push. Completed checked changes go to the authorized repo; `main` automatically
builds and publishes via `.github/workflows/firmware-pages.yml`, PRs do not deploy.
Pages is already enabled with GitHub Actions:
**https://traumlewis.github.io/Lighthouse-Bridge/**. Do not create another Site or
change visibility. Follow CI/deployment to completion, inspect real logs on errors,
rerun only failed jobs if appropriate. Verify live version, manifest and all image
checksums when firmware/layout changes. Keep README/API/WebFlasher docs and this
file aligned with the final implementation.

Version 0.3.0 software checks add 16-entry configuration/migration/state tests,
retired-group Hue behavior, OTA header/container corruption checks, dynamic UI,
copy/name actions and simulated multipart success/failure. See docs/VALIDATION.md.
No device is attached; retain the owner's earlier four-station evidence separately.

Version 0.3.1 owner report: Alexa no longer finds newly assigned station IDs 6–9.
Repeated Hue collection GETs reach the bridge with configured=4/fresh=4 initially;
BLE reads recover intermittently. This is not evidence of a radio-caused discovery
failure. Code inspection reproduces malformed/unpadded new unique IDs. Version
0.3.2 corrects the format and tests serialized discovery metadata. Owner Alexa
rediscovery remains required; no attached hardware validates the discovery fix.
Never copy the credential-bearing raw log into the repository.

## Experimental 0.3.4 web delivery follow-up

- Owner's 0.3.3 log confirms worker reservation/release, endpoints ready and HTTP
  ready; the browser still shows a completely blank page. Reported largest free
  block is about 9 KiB; the compressed page is about 17 KiB. This does not prove
  TCP pressure as the cause. Do not retain the raw log, credentials or identifiers.
- Matter dashboard delivery uses `http_body.h`: fixed Content-Length, binary-safe
  socket writes capped at 512 bytes, partial-write handling and disconnect on an
  incomplete body. Begin/end diagnostics log only byte counts, time and heap.
  Complete writes mean socket acceptance, not proven browser receipt/rendering.
- Matter default setup/host suffix uses `esp_read_mac(..., ESP_MAC_WIFI_STA)` before
  a Wi-Fi netif exists, preserving legacy suffix bytes/custom names; never persist
  a fabricated zero suffix. Stable 0.3.9 behavior remains unchanged.
- Correct private route is `/api/matter`; `/api/metter` returns NOT_FOUND. It needs
  an own-password session, not bootstrap/API key. No code is public.
- Full 0.3.4 hardware startup/web/OTA/multi-station reliability remains unverified.
- 0.3.4 local checks: both builds, ten native programs, 16 Python checks and all
  browser/USB flows pass; real localhost gzip delivery renders the login form.
  Matter app 1,965,696/1,966,080 bytes (384-byte flash reserve), static RAM 123,724.
  These are software/linker results, not measured runtime margins or device flashes.

## Experimental 0.3.5 status allocation follow-up

- Owner's 0.3.4 test renders the interface with OUT_OF_MEMORY/20. Logs confirm
  worker/endpoints/HTTP ready, one failed body write and a later complete write;
  fixed 4512-byte snapshots repeatedly fail. Do not copy raw logs or credentials.
  Heap variation alone does not prove a leak, crash or its cause.
- `StatusSnapshot` owns checked per-entry blocks <=256 bytes only for configured
  entries and existing discovery results; small metadata stays on the HTTP stack.
  Capture all entries plus uptime under one lock, release before network IO.
  Retain full capacity/fields/string ownership. Free all partial allocations and
  fail before streaming on allocation failure; never silently truncate.
- `free_data_heap` / `largest_data_block` use INTERNAL|8BIT capabilities. Existing
  Arduino heap fields keep their meaning; INTERNAL can include word-only IRAM.
  Web/status/startup data-heap diagnostics explicitly use `_8bit` labels.
- Matter UI generation maps numeric DOMException abort codes (including 20) to
  translated NETWORK_ERROR; server error strings remain intact. Stable UI unchanged.
- No task stack reductions, partition changes, new radio retries, automatic power
  writes or saved credential/identity changes. Hardware rendering/memory/OTA and
  multi-station behavior in 0.3.5 still require owner retesting.
- 0.3.5 local verification: both builds, ten native programs, 16 Python checks and
  all browser/USB simulations pass, including small-block allocation/cleanup and
  numeric AbortError normalization. Local app 1,965,952/1,966,080 bytes (128-byte
  flash reserve), static RAM 123,724 bytes; not runtime-memory validation.

## Experimental 0.3.6 discovery abort follow-up

- Owner's 0.3.5 log confirms a complete page transfer, then a real abort at discovery
  completion. The retained release ELF matches the reported ELF SHA; decoded frames
  identify `NimBLEScan::getResults` copying its advertised-device vector, failed
  `operator new`, exception allocation and terminate. Core-dump storage errors
  follow the abort; do not change partitions to address them. Do not retain raw logs.
- Matter alone uses `matter_scan.h` and pinned NimBLE public legacy `ble_gap_disc`:
  5000 ms active discovery / 2000 ms passive recovery, interval/window 97/67 ms,
  duplicate filtering. One worker owns start/wait/lifetime; the host callback only
  copies raw reports to fixed worker-owned storage. Atomic release/acquire completion
  protects that lifetime; start failures return without waiting. No second worker.
- `scan_capture.h` bounds AD-name parsing and 16-result MAC deduplication, flags
  overflow and ignores unrelated advertisers. No advertised-device objects, result
  vectors or strings are allocated by this capture path. Controller/host internals
  still consume memory; no allocation-free-radio or runtime-headroom claim.
- Recovery matches the assigned MAC, counts reports (not unique devices), classifies
  HCI types explicitly and logs RSSI/observed type, never addresses/keys. Never
  change persisted identity/address type or write power. Connections stay Random.
  Native tests cover floods, all legacy types, malformed names, duplicate/capacity
  behavior, completion and start/end errors. Failed discovery retains old results.
- No automatic power commands, stack reductions, partition/NVS/auth changes.
  Owner now confirms a completed four-result 0.3.6 discovery; save/switch, long-run
  coexistence and multi-station operation remain open. Stable 0.3.9 keeps its
  existing NimBLEScan path.
- 0.3.6 local checks: both builds, eleven native programs, 16 Python checks and all
  browser/USB flows pass. ELF symbols omit the old scan result-copy path. Local
  app 1,962,720/1,966,080 bytes (3360 flash bytes reserve), static RAM 123,716;
  these are not runtime heap margins or hardware scan validation.

## Experimental 0.3.7 HTTP/configuration memory follow-up

- Owner's 0.3.6 discovery completes with four results after five seconds and no
  shown abort. HTTP writes still stall/fail with low byte-addressable heap; saving
  reports unreachable. No save-phase evidence establishes whether it was accepted.
  Wi-Fi status/reconnect diagnostics also occur; causality is not established.
  Never retain the credential-bearing log or invent a leak/certified hardware fix.
- Matter HTTP uses the pinned WebServer's protected write hooks and request args
  via `MatterWebServer`, no private-layout casts. Move the plain body without a
  full-body copy. Checked <=256-byte nonblocking sends yield/retry transient
  EAGAIN/ENOMEM/ENOBUFS/EINTR with 2.5-second no-progress / 8-second writer budgets;
  these are not hard end-to-end network guarantees. Close incomplete responses.
  JSON has exact Content-Length; never infer full delivery merely from connected().
- Matter save owns in-place request parsing (<=8192 bytes, bounded static arena),
  copies validated configs to bounded stack storage, then frees input before NVS
  phases. No restored-state array or full config/registry JsonDocument is allocated
  on heap during save. Stream one counted/exact-sized NVS String at a time; registry
  transition checkpoint still precedes the atomic station record. All 16 entries
  and 32 bindings remain supported. Existing native identity/migration logic applies.
- Keep old live config/history paired until reboot; do not reassign while an old BLE
  read finishes. Reject occupied commands/scans, return assigned new IDs with
  `restarting:true`, pause BLE/commands and start 1.5-second grace after the reply
  serialization. New Config save phase/accepted/complete/8-bit heap logs omit secrets.
- Matter UI queues JSON requests, skips background upload polling and handles an
  accepted restart by stopping polling and checking private account reconnect up
  to eight times, two seconds apart (request time can add delay), then reload/sign-in.
  Preserve disabled power until fresh status and ignore older in-flight status errors.
- Both C/C++ Matter build flags pin NimBLE max connections to 1, with a firmware
  assertion. This matches the worker's one-client/cleanup invariant, not station
  capacity. No stacks, MTU, mbuf/transport pools, retries or scan durations are shrunk.
- `JsonStreamWriter` shares ArduinoJson code across counting/HTTP/NVS sinks without
  heap type erasure. Build-only zopfli==0.2.3.post1 (Python 3.12 requirements) emits
  standard gzip, round-trip checked. Keep C LTO disabled and both slots unchanged.
- Runtime heap/stack high-water, hardware NVS/reboot/OTA and multi-station RF behavior
  remain hardware checks. Software socket-pressure/browser simulations are not ESP32 tests.
- 0.3.7 local validation: both builds, twelve native suites, 16 Python checks and
  all browser/USB flows pass. App 1,965,440/1,966,080 bytes (640 flash bytes reserve),
  static RAM 123,268 bytes; real device runtime/NVS/stack margins remain unmeasured.

## Experimental 0.3.8 saved-station startup recovery

- Owner 0.3.7 evidence confirms save accepted=true/complete=true, then repeated
  startup aborts. The exact retained ELF matches the reported SHA prefix. Frames
  identify libc UART-lock creation while Matter cluster allocation errors are
  logged; one boot explicitly fails endpoint allocation then aborts creating a
  NimBLE client after premature worker release. This is negative multi-station
  startup evidence, not failed persistence. Never retain the raw attachment.
- Initialize NimBLE before starting Matter, with its peripheral/broadcaster roles
  compile-time disabled and central/observer enabled; keep connection count 1.
  Stacks, MTU and packet pools are unchanged. The reserved 8192-byte worker stays
  gated until both BLE and the complete Matter model succeed; failure rejects radio
  work and keeps authenticated HTTP/OTA/USB recovery. Service USB throughout; defer HTTP handling for the first three seconds
  of model construction to avoid overlapping page buffers, then service HTTP if
  initialization is still pending, and never resume endpoints before the root stack starts.
- Reserve 4096 bytes of internal byte-addressable recovery memory before Matter.
  Prime libc SDK console logging before allocation pressure and register a failed-
  allocation hook that only sets a flag under a short ISR-safe spinlock (no logging/allocation/free/IO).
  Use model_startup.h for construct/health/activate/health, then release the reserve
  before rollback/error reporting. Budget checks are policy, not measured RAM margins.
- Explicitly add Descriptor clusters to bare resumed/created bridge/light endpoints
  if absent; retain On/Off Light type and all SDK light clusters/features/identities.
  Enable only after full construction; on failure destroy non-root RAM endpoints
  through public SDK APIs, invalidate root PartsList, keep NVS identity records and
  fabrics, and do not release the worker. Never renumber or erase to recover.
- Guard Matter BLE-client creation on byte-heap availability, exposing BLE_NO_MEMORY
  with no write. Existing measured-state, bounded retries and failure/grace rules
  remain; no automatic power command is introduced. SDK/GATT runtime headroom is
  still unverified. Low-memory startup must remain explicit rather than rebooting.
- Ignore `.codex-attachments` in Git and exclude it from stable source ZIP traversal.
  Matter source ZIPs include every local .h plus build requirements. Tests must
  check privacy exclusion and complete local build inputs without storing raw logs.
- Subsequent owner 0.3.8 startup/recovery evidence is negative (see 0.3.9 below).
  Runtime heap/stack, RF coexistence and OTA remain hardware checks; no local
  ESP32 or Echo is attached.

- 0.3.8 local validation: both firmware builds, thirteen native suites, nineteen
  Python checks and all browser/USB flows pass. Matter app 1,963,088/1,966,080 bytes
  (2992 flash bytes reserve), static RAM 123,284 bytes. Runtime heap/stack,
  saved-station startup and failure recovery remain unverified on hardware.

## Experimental 0.3.9 uninitialized-cluster cleanup correction

- Owner 0.3.8 evidence is negative: repeated boot loops, usually after building
  station 1 with only 3.1–3.4 KiB free byte heap. The exact CI ELF SHA matches;
  the abort is LazyRegisteredServerCluster::Destroy via Descriptor shutdown,
  endpoint::destroy and our rollback. Recovery was not reached. No raw log stored.
- Pinned esp-matter 1.5 endpoint::destroy invokes cluster shutdown even before
  their init ran; the lazy Descriptor shutdown asserts when unconstructed.
  `include/matter_endpoint.h` owns the tested discardMatterEndpoint policy:
  for never-initialized endpoints first unlink/free all cluster metadata through
  public cluster::destroy, then endpoint::destroy on the empty endpoint. For
  initialized endpoints retain normal SDK shutdown. Propagate cleanup errors.
- Track successful enable calls explicitly on the CHIP task in bounded 17-handle
  storage; SDK is_enabled is not an init-history flag (create defaults enabled,
  resume does not). Disable each new/resumed bare endpoint before adding clusters.
  Keep root endpoint 0, mandatory light clusters, station/endpoint/fabric NVS and
  monotonic SDK allocation metadata intact. No private casts or SDK key edits.
- Before each light require its admission budget plus 2048 free byte heap; start
  at a conservative 4096-byte light cost and raise it to the largest measured
  completed light construction cost. This can reject before another SDK OOM,
  not guarantee heap sufficiency. No station truncation or reduced 16 capacity.
  Failure leaves readiness false, releases reserve and withholds the BLE worker.
- 0.3.9 targets the established rollback abort, not proven multi-station RAM
  sufficiency. Actual saved-station boot/recovery, BLE, OTA and memory margins
  remain unverified on hardware. Stacks/MTU/pools and the exact slots are unchanged.

- 0.3.9 local validation: both builds, thirteen native suites (including the
  uninitialized-shutdown regression), nineteen Python checks and all browser/USB
  simulations pass. Matter app 1,963,600/1,966,080 bytes (2480 flash bytes reserve),
  static RAM 123,284 bytes. These are not runtime memory margins or device tests.

## Independent rs-matter prototype
- Installed **0.1.17-rs-matter** polishes only the rs Wi-Fi setup presentation.
  The owner's 0.1.16 Android retest confirms joining the setup AP and displaying
  the portal; the screenshot shows the setup root, not evidence of permanent
  Local Mode/BLE operation. The underlying 0.1.15 failure cause stays unproven.
  Replace the rs root's vendor menu with local EN/DE HTML in rs_wifi_portal.h:
  clear home/local choice cards with descriptions and visible gaps, a compact
  broadcast-SSID panel and secondary Info/Leave setup actions. Do not display
  a misleading home-disconnected error on the normal setup landing page. Preserve
  actual failure/status messages on vendor Wi-Fi configuration/info pages.
  A narrow derived WiFiManager wrapper forwards the pinned protected captive
  redirect and request-timestamp hooks before custom rendering. Register the
  custom root GET before vendor routes, retain /wifi /local /info
  /exit behavior and all existing raw-BIN blocks, token/Origin checks and clean
  setup boot. Root and Local Mode confirmation/restart share the dark theme;
  language selection persists only the existing language preference and continues
  across those pages. No public API keys/passwords/codes or new remote assets.
  No changes to radio/auth/NVS/mode selection/OTA/identity logic. Preview images
  use synthetic SSIDs only. The owner separately approved OTA from 0.1.16;
  installation succeeded in home mode. Ask before future updates or setup/mode
  activation; general cleanup remains deferred.
  Verification: full thirteen browser/USB programs and 31 Python checks pass.
  Actual local root/confirmation/restart templates are exercised EN/DE at
  1280/390/320, including gaps, >=44px secondary targets, no overflow, selected
  language continuity and GET-only navigation to unchanged destinations. Synthetic
  preview PNGs live under .pio/rs-0117-portal-*.png; no private screenshot was
  copied into source/previews. Final app 1,950,896/1,966,080 bytes; static RAM
  67,580. Exact partitions/chip/OTA/hash/libc/Wi-Fi guards pass; ELF retains
  14 KiB arena, 52,528-byte Stack and 18 KiB task. Archived 0.1.16 comparison
  confirms version-only Rust changes and unchanged radio/protocol/auth/config/
  identity helpers/base dashboard. The source diff changes only setup rendering;
  actual owner-approved OTA reports 0.1.17, home Wi-Fi connected, Matter
  ready/paired and usable BLE. Four assignments and OFF choices, credentials
  and naming match pre-update values. Real guarded Chromium passes EN/DE home
  layouts at 1280/390/320, enabled controls, debug-only refresh and ready/paired
  Matter information, with no forbidden mutations or page errors. Diagnostic
  sessions are closed. The new setup portal was not activated by the agent;
  the owner subsequently confirms the 0.1.17 portal works on the device.
  This does not establish permanent Local Mode/BLE or long-run operation.

- Version **0.1.16-rs-matter** addresses the owner's Android rejection of the
  setup-AP password in 0.1.15. Read-only checks verified the supplied API key is
  accepted by the running bridge and its setup SSID matches the reported WLAN;
  forgetting/rejoining did not help. This is negative setup-connection evidence,
  not evidence that the permanent Local Mode was entered. Never commit the real
  key, SSID or raw diagnostic responses. Root cause is not yet established.
  Espressif's pinned-generation ESP32 coexistence guide labels SoftAP connecting/
  connected with BLE as C1 (supported but unstable):
  https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-guides/coexist.html
  This supports isolating setup, not a claim that RF contention caused this case.
  RS setup requests now persist only a one-shot lhbridge/wifi_setup boolean,
  pause new work, wait physical BLE idle and schedule reboot. Successful enqueue
  is required before accepting the request; failures preserve running state.
  BOOT and the dashboard use this path. On boot consume/clear the flag before
  Wi-Fi initialization; if clearing fails, log and use the normal saved mode
  rather than creating a repeated-setup boot loop. Do not change wifi_mode,
  router credentials, API key/web auth, stations, endpoints or fabrics.
  A requested setup starts AP-only, without auto-reconnecting STA, before BLE
  or Matter initialization. The same protected menu/token POST and 180-second
  setup timeout apply. Closing/timing out returns to the saved mode by reboot;
  saving home/local retains its established explicit selection behavior.
  Keep the API key visible/copyable while the browser hands over to setup,
  stop polling and retain clear EN/DE restart instructions. No automatic power.
  Read-only LH_WIFI_INFO over physical USB reports numeric AP mode/config error,
  auth/cipher/channel/PMF/client counts, key_matches/name_matches booleans, event
  counts/reason and byte-heap diagnostics; never raw passwords, SSIDs or MACs.
  Wi-Fi event context stores fixed atomic numbers only; serial output stays on
  the main loop. Print a snapshot when setup/local AP starts. Existing USB readers
  keep sending only their previous info commands; do not auto-send recovery.
  Permanent Local Mode BLE scheduling is unchanged. Its AP/BLE coexistence and
  Android connectivity remain hardware checks; do not claim a fix from a build.
  The owner separately approved OTA from 0.1.15; it succeeded in home mode.
  Future OTA and actual setup/mode activation require separate approval.
  Verification: 18 native programs, eight Rust tests in debug/release, all 31
  Python checks and all thirteen browser/USB programs pass. Mode tests exercise
  enqueue/consume exactly once, cancellation/timeout next boot, persistence
  failure and preservation of unrelated records. EN/DE clean-setup UI tests cover
  confirmed restart, cancellation/storage error without handoff, copyable API
  key across rebooted 401s, stopped polling and no implicit power/mode requests.
  Stable and SDK builds pass. Final rs app 1,942,832/1,966,080 bytes, static RAM
  67,588. Exact partitions/chip/OTA/hash/libc/Wi-Fi guards pass; ELF retains
  14 KiB arena, 52,528-byte Stack and 18 KiB task. Archived 0.1.15 comparison
  confirms Rust protocol/transport, station/state/config/identity helpers and base
  HTML unchanged after version normalization; dependency lock changes root only.
  Owner-approved real OTA reports 0.1.16, home mode, connected Wi-Fi, Matter
  ready/paired and usable BLE. Four assignments and OFF choices, credentials and
  naming match pre-update values. Real guarded Chromium passes EN/DE layouts at
  1280/390/320, normal home controls, ready/paired Matter information and both
  clean-setup restart confirmations (cancelled). No setup/mode/power/config or
  commissioning POST occurred, and no page errors; sessions are closed.
  The agent did not activate setup. The owner subsequently confirms successful
  Android setup-AP connection and supplies the root-page screenshot. This is
  positive setup access evidence, not proof of permanent Local Mode/BLE operation
  or of the original password-rejection cause.

- Version **0.1.15-rs-matter** adds the owner-requested permanent Local Mode
  selected from WiFi Setup -> Local Mode -> dashboard. The owner separately
  approved OTA from 0.1.14; it succeeded. Ask before future OTA or an actual
  network-mode switch. The board remains in home mode; cleanup stays deferred.
  Persist only lhbridge/wifi_mode=home|local as one atomic record, default home
  for old/unknown values. Keep router credentials, web auth, API key, stations,
  endpoint registry and fabrics. Setup selection uses a token-protected POST
  with Origin checks, not a mutating GET, inside the existing protected setup AP.
  A separate confirmation page explains no internet/Alexa and the restart.
  Reboot into WPA-protected AP-only Wi-Fi using the saved setup SSID/API key,
  fixed 192.168.4.1/24, channel 1 and at most four clients. Do not auto-connect STA
  or erase its credentials. AP startup failure exposes setup recovery without
  overwriting the saved choice. Setup's successful home-Wi-Fi save restores home
  mode; timed-out/cancelled setup returns to the saved mode on reboot.
  Start HTTP, the unchanged sequential BLE worker and registry checkpointing in
  Local Mode without starting/polling an rs-matter task/model/network stack. Keep
  the worker gate, modem-sleep coexistence policy, startup/periodic read-only work,
  ON/OFF preferences, authentication/API/OTA and 16-station editor. Never send
  power automatically on a mode switch or restart. Registry corruption still
  fails closed; no identity/fabric clearing. Local station saves use the same
  checkpoint/persist/restart path. Existing pairings resume on returning home.
  Local status/account/private Matter info expose network_mode:local; status IP
  and USB URL use the AP address. wifi_connected continues to mean STA connection.
  LOCAL_MODE/ready=false/paired=false describe the paused runtime, not deleted
  fabric data. UI says Matter paused, suppresses code/renewal and misleading
  unpaired messaging, but keeps BLE/OTA controls usable. Local HTTP mDNS must
  never stop an active Matter DNS-SD responder in home mode.
  Personal-session POST /api/network {mode:home} restarts into saved home Wi-Fi
  (or setup when unavailable); API keys/bootstrap cannot switch modes. Reject
  pending power/scan/OTA work and preserve the running mode on persistence failure.
  Local AP key regeneration and naming changes restart to apply AP credentials;
  block them during power/scan/OTA work. Keep the new key visible/copyable across
  rebooted 401s until the owner reconnects/reloads, clear it on explicit logout,
  and stop polling during network handoff. Do not put keys/tokens in URLs/storage.
  Serial parser accepts local-mode URL with wifi_connected=false only when the
  mode is explicit, retaining strict IPv4 HTTP URL validation. USB Matter panels
  explain paused Local Mode in EN/DE; cache-bust changed parser/copy imports.
  Stable 0.3.9 and SDK 0.3.9-matter retain home-network behavior and both existing
  app slots/complete partition tables. Actual standalone AP/BLE coexistence,
  AP OTA and home/AP hardware round-trip remain unverified until owner tests.
  Software verification passes: all 18 native test programs, eight Rust tests
  in debug/release, 31 Python checks and all twelve browser/USB programs. Native
  mode tests cover legacy/invalid home defaults, exact local selection, failed
  persistence and unrelated settings preservation. EN/DE mobile-emulated browser
  coverage exercises the actual confirmation/dashboard HTML, token POST without
  query credentials, local BLE controls despite paused Matter, return-home cancel/
  failure/restart and key/name handoff including rebooted 401, copying/logout.
  USB parser tests cover explicit local addresses and reject inconsistent modes,
  unsafe URLs and local STA-connected frames; USB UI explains LOCAL_MODE.
  App 1,940,736/1,966,080 bytes (+10,384 over 0.1.14), static RAM 67,572 (+32).
  Exact partitions/chip/OTA/hash/cache-safe libc/Wi-Fi guards pass. ELF retains
  14 KiB arena, 52,528-byte Stack and 18 KiB task. Rust/transport, BLE/state/config/
  identity helpers compare unchanged with archived 0.1.14 after version metadata
  normalization. Cargo.lock changes only the root package version. Stable and
  SDK builds also pass; SDK app 1,963,792/1,966,080 bytes, static RAM 123,284.
  Real owner-approved OTA reports version 0.1.15, network_mode:home, connected
  STA, Matter ready/paired and usable BLE. Four assignments and OFF preferences,
  credentials and naming match pre-update values. Real guarded Chromium passes
  EN/DE at 1280/390/320: existing theme/status/control layout, usable BLE buttons,
  Local Mode setup help, hidden return-home button in home mode, debug-only
  refresh and the ready/paired Matter panel. No power/config/network-switch/
  commissioning request or page error occurs. Diagnostic sessions are closed.
  No AP activation, power, key/name change or new pairing window was requested.
  Actual standalone AP/BLE/OTA and a home/AP round-trip remain hardware checks.

- Version **0.1.14-rs-matter** reverts only 0.1.13's light/black language
  selector override to the preceding theme-native appearance. Preserve its
  Sign out dimensions and debug-only Read current states behavior. No radio,
  identity, auth or config changes. The owner explicitly approved its OTA;
  installation from 0.1.13 completed successfully. Ask before subsequent updates.
  Verification: account/preferences and presentation/OTA-reload browser programs
  pass, confirming preceding theme colors, preserved dimensions and debug-only
  refresh. All 31 Python checks pass. App 1,930,352/1,966,080 bytes, static RAM
  67,540 unchanged. Partition/chip/OTA/hash/libc/Wi-Fi guards pass; ELF arena,
  Stack and foreground task remain 14 KiB / 52,528 bytes / 18 KiB. Real post-OTA
  status reports 0.1.14, Matter ready/paired, usable BLE and no startup error.
  Four assignments, OFF preferences, credentials and naming are retained. Real
  Chromium checks pass EN/DE at 1280/390/320: preceding language colors restored,
  matching control dimensions, no overflow and debug-only refresh across language
  changes, with no debug-triggered read. No power/config/commissioning request or
  page error occurs. Diagnostic sessions are closed.

- Version **0.1.13-rs-matter** adjusts only the rs dashboard presentation and
  version metadata. The language selector and its options use black text on a
  light background with color-scheme:light, preserving Sign out sizing. Manual
  Read current states is hidden by default and shown only in debug; language
  changes preserve it, logout resets it. Debug toggling queues no reads or power;
  existing startup/periodic/background reads remain unchanged. Shared stable/SDK
  HTML and radio/auth/config/protocol logic are unchanged. The owner separately
  approved OTA to 0.1.13; it completed successfully. Ask before future updates.
  Verification: targeted account/preferences, presentation/OTA-reload and Matter
  gzip dashboard browser programs pass, including black selected/option text on
  light background, hidden/default and visible/debug refresh, no debug-triggered
  read and unchanged control dimensions. All 31 Python asset/layout/archive
  checks pass. App 1,930,368/1,966,080 bytes (+32), static RAM 67,540 unchanged;
  exact partitions/chip/OTA/hash/libc/Wi-Fi guards pass. ELF arena/Stack/task stay
  14 KiB / 52,528 bytes / 18 KiB. Real OTA from 0.1.12 reports 0.1.13,
  Matter ready/paired and usable BLE. All four assignments and OFF choices,
  credentials and naming are retained. Real Chromium checks pass EN/DE at
  1280/390/320: black selected/option text on light background, matching control
  dimensions, refresh hidden normally and visible in debug across language
  changes, and no read queued by debug toggles. A request guard detects no power,
  configuration or commissioning mutation; no page errors. Sessions are closed.

- Version **0.1.12-rs-matter** completed its explicitly owner-authorized OTA.
  This permission covers this update, not subsequent firmware changes or tests
  that change credentials, station settings, power or pairing. General cleanup
  remains deferred. The installed board reports 0.1.12, Matter ready/paired and
  BLE ready, with all four assignments and credentials retained.
  The rs-only dashboard places connection status RIGHT of Lighthouse Bridge,
  sizes the language select like Sign out, and uses correct English Base station
  terminology without renaming saved station names. Cards show Alexa name first;
  STATION ID, MAC and technical read history appear only in debug. Contrary to
  the earlier fixed-height requirement, the owner now wants rs cards to grow and
  shrink when debug toggles; normal pending/unavailable feedback remains visible.
  Mobile discovery uses a native select alongside the manual MAC input, updated
  without overwriting edits. EN/DE and keyboard support remain required.
  `off_state` in station config/status/NVS is sleep or standby, default sleep;
  omitted fields preserve existing choices on edits, old records migrate to sleep.
  Alexa OFF/toggle-off and dashboard individual/all OFF use each saved preference.
  Local API state off follows preferences; explicit sleep/on/standby remain exact.
  STANDBY is confirmed only by measured raw 2, SLEEP only by 0, ON only by 11.
  Coalescing, generation guards, bounded read-only confirmation and no automatic
  boot/reconnect power writes remain unchanged. Stable/SDK OFF stays SLEEP.
  First setup has no username. Optional username defaults disabled for old/new
  records; personal-session POST /api/username enables/disables/changes it, stored
  atomically with the existing web_auth salt/hash. Password changes preserve it.
  Usernames use 1..32 ASCII letters/digits/dot/underscore/hyphen. Login requires
  the matching name only when enabled after bootstrap; physical web-auth recovery
  clears it with the password. Settings changes invalidate other web sessions.
  Public GET /api/login exposes only username_enabled, never the name or secrets.
  Personal-session POST /api/key with confirm:true regenerates access_key, also
  changing the future setup-AP password. It leaves router Wi-Fi, own password,
  sessions, stations and Matter fabrics intact. Old integration keys immediately
  stop authenticating. Both new mutation routes require personal non-bootstrap
  session and Origin/JSON checks, never API key authorization. Browser rotation
  asks confirmation; key/name remain page memory only and clear on logout.
  Account labels are Password and username settings / Passwort und Benutzername
  ändern, with Password / Passwort as the password action. No actual key/name
  rotation, standby power test or new commissioning window is authorized merely
  by permission to install this version. Preserve both app slots and identities.
  Legacy auth records missing username load an empty value, never the String
  conversion of JSON null. Native regression coverage includes missing/null/non-
  string names, strict username validation, 16 maximum-size station configs,
  omitted-field migration, exact standby confirmation and Matter OFF dispatch.
  All 17 native test programs, eight Rust tests in debug/release, 31 Python tests
  and eleven browser/USB programs pass; stable PlatformIO also builds. EN/DE
  mobile-emulated browser tests cover username enable/rename/disable/sign-in,
  rejected saves, confirmed/cancelled/failed key rotation, native selection,
  language/edit preservation and adaptive debug cards. These credential/standby
  mutations are software-tested, not performed on the owner's board.
  App is 1,930,336/1,966,080 bytes, static RAM 67,540 (+16 over 0.1.11).
  Exact partitions/chip/OTA/hash/libc/Wi-Fi guards pass; arena/Stack/task remain
  14 KiB / 52,528 bytes / 18 KiB. Real OTA preserves the key/password/naming,
  four assignments, paired status, SLEEP defaults and command histories none.
  Public login exposes only username_enabled:false; the private name is empty.
  New account mutation routes reject wrong Origin and API-key-only access on
  the real board. Diagnostic sessions are closed. Real Chromium checks pass
  EN/DE at 1280/390/320 px: right-side status, matching language/Sign out controls,
  growing/shrinking debug cards and visible account/OFF/native-select controls.
  A guarded pre-update 0.1.11 tab automatically loads 0.1.12 after external OTA;
  no forbidden power/config/commissioning request occurs. Own-upload reconnect
  cases remain software-tested. Physical standby writes, actual key/name changes,
  specific phone browsers, long-run and sixteen-station runtime remain unverified.

- Installed **0.1.11-rs-matter** completed the separately owner-approved OTA
  from 0.1.10. Keep the existing published installer until publication is
  separately requested. The owner approved the display/product/commissioning name
  Lighthouse-Bridge, dropping the old Matter Test suffix; never overwrite saved
  station names, hostname, identifiers, fabrics or controller-assigned labels.
  Removing the suffix does not replace test attestation or imply certification.
  In the rs-only generated dashboard, put connection status left of the top
  Lighthouse Bridge branding, with EN/DE desktop/mobile layouts. Measured ON
  keeps its green colors; Unknown is gray, SLEEP blue, STANDBY orange. Select
  colors from measured/reachable state, never from requested targets; retain
  badge/status dimensions, text, keyboard support and contrast.
  After an acknowledged browser OTA, stop polling, wait for the reachable
  post-reboot 401 login gate (old authenticated 200 is not readiness), and reload
  once. Bound reconnect attempts to 90 seconds with readable manual-reload
  fallback; never retry the upload. Also detect externally initiated updates:
  compare authenticated status.version against the loaded page; after session
  loss inspect the public page version and reload only if it changed. An ordinary
  session expiry must not reload unchanged firmware. The new behavior requires
  the new HTML to have been loaded once; an old 0.1.10 tab needs an initial refresh.
  Keep personal-session/code privacy, the one action pairing UI, 16-station
  capacity, ON/SLEEP-only commands, exact flash layout and buffer/stack limits.
  General cleanup remains deferred. New browser coverage is part of npm test.
  Software validation: all eight Rust tests pass in debug/release, 31 Python
  checks and all ten browser/USB programs pass. EN/DE checks at 1280/390/320 px
  cover measured badge colors/contrast >=4.5, no target-induced color changes,
  no card-height jump, top-left status position and no horizontal overflow.
  OTA scenarios cover rejected upload/no reload, old server 200, downtime,
  rebooted 401, external update, unchanged expiry and authenticated version
  mismatch without re-upload. App 1,925,824/1,966,080 bytes (+480 over 0.1.10),
  static RAM 67,524 unchanged. ELF retains 14 KiB arena, Stack 52,528 and CPU
  task 18 KiB. Exact partitions/chip/OTA/hash/libc/Wi-Fi-wrapper checks pass.
  Archived 0.1.10 comparison confirms Rust logic and shared C++/base HTML are
  unchanged after normalizing only Rust name/version metadata. Real OTA returned
  installed/restarting, then version 0.1.11, Matter ready/paired, a usable BLE
  worker and no startup error. Four assignments, credentials and naming are
  retained; command histories remain none. Real Chromium checks pass EN/DE at
  1280/390/320 px: status left of branding, no horizontal overflow, measured
  SLEEP blue, automatic private status on explicit opening and Lighthouse-Bridge
  name. Paired status is retained, window closed and code absent. A request guard
  confirms no power/config/commissioning mutation; diagnostic sessions are closed.
  All four physical readings were SLEEP; other colors and the new OTA reload
  flow remain software-tested. An old tab needs one refresh to load that flow.
- Version **0.1.10-rs-matter** simplifies only the rs-matter dashboard's
  Matter section. The owner approved OTA from 0.1.9; it completed successfully.
  Opening the section issues a personal-session GET /api/matter; neither login
  nor a collapsed panel fetches the code. Only the remaining explicit action
  POSTs /api/matter/commissioning. Remove the obsolete show control and its
  translations; retain the transient code's copy button and existing bridge name.
  Closing stops private polling and clears displayed name/status/code; late
  replies cannot restore a hidden code. Reopening queues a fresh read if an
  earlier fetch is pending. Preserve explicit renewal requests and avoid control
  flicker during automatic polling. Logout closes the section and invalidates
  late replies, so relogin cannot implicitly retrieve the code. Keep EN/DE copy.
  The shared SDK prototype/stable UI and Rust/BLE/network/identity logic are
  unchanged apart from the rs-matter firmware version metadata. User approval
  remains required before any further device update or new pairing action.
  Software verification: eight Rust tests in debug/release, 31 Python checks
  and all nine browser/USB programs pass. Targeted EN/DE browser tests also cover
  closed-panel late replies, stale polling callbacks, reopen-during-fetch and an
  explicitly queued renewal completing after collapse. The app is
  1,925,344/1,966,080 bytes (+64 versus 0.1.9), static RAM 67,524 unchanged.
  ELF retains the 14 KiB arena, 52,528-byte Stack and 18 KiB foreground task;
  exact partition/chip/OTA/libc/Wi-Fi-wrapper/arena checks pass. Archived 0.1.9
  sources confirm Rust/C++ protocol, worker and adapter code is unchanged after
  normalizing only version metadata. The real post-OTA board reports version
  0.1.10, Matter ready/paired, BLE worker available and no startup error. Four
  assignments, API key, personal password and naming match pre-update values;
  command histories remain none. Real Chromium checks on that board pass EN/DE:
  no show button, no private read before explicit section opening, automatic GET
  on open/reopen, correct paired/closed-window status, and clearing on collapse.
  A request guard confirms no commissioning/power/config/firmware mutation during
  the browser check. Active-code renewal behavior is software-tested, not retested
  on hardware because no new pairing window was authorized. Sessions are closed.
- The owner replaced the Tasmota experiment with rs-matter; **0.1.18-rs-matter**
  is now installed on the owner's board. The authorized public rs-matter release is 0.1.18;
  publish through the existing checked main-branch workflow. Tasmota's source,
  Berry runtime, builder, dedicated tests and public installer are removed. Stable
  0.3.9 and the existing SDK prototype 0.3.9-matter remain separate and unchanged.
- `experimental/rs-matter/` combines the shared C++ firmware and BLE worker with
  a no_std Rust static library. Pin project-chip/rs-matter revision
  43f61585a439f1a1e90924b696a6280850ef0145 and commit Cargo.lock. Pin Espressif Rust
  1.97.0.0 / espup 0.19.0, Arduino ESP32 3.3.12 and the existing library versions.
  Board profiles select xtensa-esp32-none-elf, riscv32imc-unknown-none-elf or
  xtensa-esp32s3-none-elf and link only the matching chip's app.
  No Tasmota/Berry or C++ CHIP protocol stack is linked into this prototype.
- Preserve the exact partition table, both 1.875 MiB app slots, four USB parts
  (bootloader 0x1000 classic / 0x0000 C3/S3; other offsets unchanged),
  own-password sessions, API key, OTA validation and stable station assignments.
  `lhmatter/endpoints` keeps the existing registry/identities and aggregator 2.
- Rust owns Matter fabrics, ACLs and protocol state in independent `lhrsmatter`
  NVS blobs, committed before successful persistence returns. Former engine fabric
  formats are incompatible: pairing again is required when changing engines.
  Do not erase prior NVS/fabrics or the old data partition automatically.
- Commission only over the saved WLAN connection. The Matter network driver does
  not initialize or release Bluetooth. The single production NimBLE worker stays
  its sole owner. Matter commands queue measured-state ON/SLEEP/Toggle through
  the shared command policy; no boot/reconnect/virtual optimistic power writes.
- Rust polling and FFI entry points run only on the foreground Arduino task.
  No FFI polling from the BLE worker/interrupts; global Rust state cannot be used
  concurrently or re-entrantly. Networking is nonblocking and network IO holds no
  stateMutex. The executor polls every foreground loop and drives Embassy timers.
- Root/aggregator/bridged Descriptor metadata use actual retained endpoint IDs,
  not array positions. OnOff and Reachable read current C++ measurements and
  notify changes from the Rust handler; command acceptance never changes them.
  Node::endpoints must be strictly ascending by ID for the pinned SDK's resumable
  IM path expander. Sort the metadata vector in place after construction; never
  renumber endpoint identities or reorder the station/configuration state arrays.
  Keep direct and wildcard real-SDK Descriptor-read coverage in debug and release
  host tests, including full, sparse, shuffled and empty configurations.
- Manual private setup-code renewal remains personal-session/Origin/JSON gated,
  coalesces pending requests and opens a fresh 15-minute Basic window. Track the
  exact window identity; closed, pending or unknown enhanced windows expose no
  cached code. Codes remain transient and clear with logout/restart.
- `tools/build_rs_matter.py` builds/packages split images, OTA/checksums/source ZIP
  in `dist/rs-matter`; `site/rs-matter.html` replaces the Tasmota page. Validate
  complete partition equality, app-slot bounds and identical OTA payloads before
  offering artifacts. Keep Rust dependency resolution locked in CI.
- rs-matter owns DNS-SD and advertises the saved custom hostname; never stop it
  via MDNS.end(). Retain all usable interface IPv6 addresses and re-read network
  state after reconnect. HTTP remains accessible by IP for recovery.
- Full-build, browser and host checks are software evidence only. Direct LAN
  testing establishes read-only PASE/Descriptor traffic on 0.1.8 and one real
  authenticated OTA to 0.1.9. No USB/Echo app is attached: safe runtime margins,
  prolonged 16-station coexistence remain unverified. The owner confirms complete
  Alexa pairing on 0.1.9; synchronization observations are tracked separately.
  The former Tasmota owner's
  GS014 report does not establish rs-matter compatibility or a verified fix.
- Published 0.1.8 baseline: app 1,924,864/1,966,080 bytes, static RAM 67,524 bytes; six
  real-Rust host tests (startup/persistence/window/expiry/sparse model with host
  shims), sixteen native C++ programs, 31 Python checks and nine browser/USB
  programs pass. The Arduino foreground stack is 18 KiB; BLE remains 8 KiB.
  Explicit Matter timed commands issue one expiry request, cancelled if a newer
  command generation supersedes it; they never create a startup timer or resend.
- The saved Wi-Fi configuration remains owned by Lighthouse. Seed rs-matter's
  Network Commissioning store from it; never acknowledge a connection to different
  credentials as successful just because the current link is connected. Reapply
  IPv6 policy and modem sleep after connection/reconnection without power writes.
- CI allows 45 minutes for all three firmware variants and native Rust tests.
  The pinned espup installation uses GitHub's built-in job token for release API
  requests, avoiding unauthenticated shared-runner rate limits. Never log it.
  Source archives exclude Rust target directories and private attachments and
  include dependency licenses. Packaging refuses stale source fingerprints.
- No commissioning window is advertised on startup. The personal-session renewal
  action opens it only after both rs-matter and the BLE worker are ready.

- 0.1.0-rs-matter owner hardware evidence is negative: cache-error boot loop
  before setup/HTTP. Exact published ELF maps the fault to Rust weak memmove
  during ESP-IDF flash initialization. 0.1.1 explicitly resolves SDK libc before
  Rust, then links Rust as a normal archive within the rescan group; never put it under whole-archive before
  SDK archives. Keep ESP-IDF cache/PSRAM-safe libc. Packaging checks final strong
  ROM/IRAM memcpy/memmove/memset/memcmp/strlen, including skip-compile, and refuses
  missing/weak/flash definitions. No ROM-address overrides, partition or NVS edits.
  Recover the boot loop with four USB parts, erasing unchecked. Owner 0.1.1 reaches Matter/BLE ready, but HTTP remains unavailable; do not infer
  HTTP or Alexa success from ELF checks.

- 0.1.1 owner log: after WLAN, Matter/BLE ready; free data heap ~2412 bytes,
  largest 2036, later free 1652/largest 916. No cache panic is shown. A later
  POWERON_RESET does not prove an OOM reboot. No raw log or identifiers in Git.
- 0.1.2 reserves a 16 KiB ESP32 Rust arena (24 KiB on 64-bit host) and 16 KiB
  foreground stack instead of 24+24 KiB, releasing 16 KiB fixed RAM. The owner
  measured foreground watermark >=18612/24576 (<=5964 bytes used) at startup/idle;
  pairing/OTA peaks remain unknown. BLE stays 8 KiB. The sole wifi_oper_task
  future fits its arena with alignment+1 KiB reserve via an exact concrete-type
  compile assertion in the driver. Log arena used/capacity, retain all protocol
  capacities and 16 stations, no heap/private-layout tricks or SDK fork.
  Host startup/window/expiry tests exercise the same driver with host-sized arena.
  HTTP/OTA, pairing and sustained RAM headroom remain hardware checks.

- Owner 0.1.2 now confirms UI discovery/assignment/save for four stations. The
  logged accepted config-save restart and session loss are intentional. Retained
  stations read SLEEP; no commissioning/power operation is established here.
  One explicit later panic matches the published ELF: Wi-Fi/coexistence logging
  reaches UART/newlib mutex initialization and aborts on failed allocation.
  Heap minimum 92 bytes and BLE_NO_MEMORY show pressure; stable later samples
  do not establish a leak or identify the transient request. Other POWERON_RESET
  entries are not proof of software crashes. Do not retain raw logs/identifiers.
- 0.1.3 warms the shared UART/VFS write mutex via public console write before
  Wi-Fi, while RAM is available. HardwareSerial bypasses that lock. Keep the SDK
  formatter, levels and messages unchanged, including float/long-long support.
  An allocation-free IRAM heap-failure hook prints numeric
  bytes/caps/count through ROM with DRAM format/counters and 1-second throttling;
  ignore the SDK function-name pointer. Include all events in periodic count.
  Keep the hook non-inlined: GCC LTO otherwise discarded its IRAM placement.
  Packaging validates final IRAM/DRAM symbols, including skip-compile. Native
  tests cover console warmup/failure, ignored invalid pointer, throttle/wrap
  and hook errors.
  Preserve 16 KiB foreground/arena and 8 KiB BLE stacks, all capacities, NVS and
  partitions. Underlying pressure, sustained stability, OTA and pairing remain
  hardware checks; this logging-path correction is not a general OOM fix.

- Owner 0.1.3 excerpt has no panic but >24,000 allocation failures, 180-byte
  heap minimum and recurring BLE_NO_MEMORY. 1528/2308-byte INTERNAL|DEFAULT and
  442-byte INTERNAL|DMA|8BIT requests fail; caller/unique cause is not established.
  Pressure begins with a closed window; later open/paired=false persists and
  Alexa reports device not found. Do not claim the open window proves discovery.
- 0.1.4 wraps public esp_wifi_init arguments after Arduino's hard-coded defaults:
  dynamic TX<=4, RX target 8 while preserving static RX/Block Ack minima and
  stricter existing limits. Keep all other public config fields/magic/callbacks,
  aggregation, static buffers, Wi-Fi credentials and modem sleep unchanged. No
  SDK fork/private struct casts. Log actual counts and validate the linked wrapper.
  Lower peak throughput is an intentional tradeoff; hardware results are open.
- UDP EAGAIN/EWOULDBLOCK/ENOMEM/ENOBUFS/EINTR yield asynchronously (send 10 ms,
  bounded 1.5 s retry window; receive 5 ms). Hard/short/timeout sends fail, never
  false ACK or split datagrams; cancellation retains the socket. Host tests drive
  the actual Rust adapter through shortage/success/hard/short/timeout/cancel.
  Report UDP TX/RX/retry/hard-send-error/timeout, mDNS socket TX/RX and IPv6 count
  without payloads/identifiers. Local syscall success does not prove Echo receipt.
  Keep 16 stations/all protocol capacities, stacks, NVS, partitions and BLE worker
  policy. Alexa discovery, routing/isolation, actual OTA and sustained RAM margin
  remain hardware checks. Do not copy raw owner logs or credential frames.

- Owner 0.1.4 confirms Wi-Fi 4 static RX/8 dynamic RX/4 dynamic TX/BA 6 and IPv6
  count 1, but UDP tx=4/rx=0 and mdns_rx=0 stay fixed with an open window. Heap
  minimum 192/236 and BLE_NO_MEMORY remain. No explicit panic in the excerpt.
- 0.1.5 fixes the adapter's false receive readiness. wait_available must wait for
  actual socket data before rs-matter acquires its shared RX buffer. The old
  unconditional 5 ms success let an empty Matter socket hold RX and starve mDNS.
  A full-stack inbound DNS PTR regression fails old (0 packets consumed) and
  passes fixed (query consumed + Basic Commissioning response). Prior send-only
  announcement/code probes did not validate receive progress.
- Use one-byte non-consuming recv(MSG_PEEK|MSG_DONTWAIT), with 5 ms waits for
  empty/transient cases, true readiness for zero-length UDP too, and fatal error
  propagation. Native socketpair tests cover empty/ready/repeated peek, intact
  payload, zero-length and closed sockets. Keep all stacks, Wi-Fi bounds, protocol
  capacities, endpoint identities, NVS and flash layout. No extra poll/select
  allocation, SDK fork or private casts. Full hardware Alexa discovery/pairing,
  multicast routing and heap-pressure improvement remain unverified. Do not
  claim the host-shim regression is a physical network test or commit raw logs.

- Owner 0.1.5 evidence confirms incoming mDNS and Matter traffic. One attempt
  adds a fabric while the window remains open and still ends in Alexa GS014.
  A fresh excerpt without a version header starts with a retained fabric/closed
  window and later changes
  to no fabric/open window, with zero allocation failures, minimum data heap
  11412 and no explicit panic. These excerpts do not identify who removed the
  fabric or prove a successful CommissioningComplete. Do not prescribe a RAM
  fix/reset, or label has_fabrics as a confirmed Alexa pairing.
- 0.1.6 is a diagnostic release, not an established GS014 fix. Enable upstream
  info/warn/error logging through log, with debug/trace compiled out. The adapter
  formats at most 256 bytes locally and emits ONLY fixed allowlisted event/error
  labels, never SDK text, keys, certificates, addresses, node/fabric IDs or bodies.
  Queue 32 small numeric records; report overflow explicitly. Serial/ROM output
  drains after polling returns, outside all stack/queue critical sections. No
  unbounded allocations, SDK forks or disabled protocol validation.
- Public SDK state snapshots log changes in fabric count, active PASE/CASE session
  counts, failsafe armed/disarmed and window state. A stored fabric alone still
  does not prove completion. Known events distinguish AddNOC, completion request
  and persistence, RemoveFabric, failsafe expiry and CASE/exchange errors.
  All stacks, Wi-Fi bounds, capacities, BLE/power policy, credentials/identities,
  NVS and exact partition table remain unchanged. Alexa failure cause and actual
  commissioning/OTA reliability remain hardware checks.

- Owner 0.1.6 log now confirms a CASE session, CommissioningComplete persistence,
  failsafe disarmed and window closed at 20:46:26, with no allocation failures.
  A preceding RemoveFabric request is explicitly handled before the fresh PASE
  attempt. UnsupportedCommand/Cluster probes occur during setup; an
  UnsupportedEndpoint read follows completion. These labels alone do not prove
  their relationship to Alexa GS014. Foreground watermark falls to 788 bytes;
  do not claim stack/RAM margins or shrink any stack from this evidence.
- 0.1.7 corrects our metadata order root 0 / aggregator 2 / station 1, which
  violated the SDK's sorted-Node contract. A real parsed Descriptor read through
  the SDK path expander fails its debug assertion on that legacy order; release
  coverage verifies that endpoint 1 is silently omitted from wildcard reads. Earlier tests
  checked construction order and direct lookups, bypassing that expander. Fixed
  metadata preserves every ID, binding and station-indexed state. Full Alexa
  discovery/app acceptance, external status and power control remain hardware
  checks even though protocol commissioning is now observed on owner hardware.
  All five Rust host tests pass in both debug and release profiles; the exact
  former model fails the new SDK-read regression before the fix. The classic
  app remains within 1,966,080 bytes. Tests are software evidence, not Echo app
  acceptance or a hardware switching test.

- Owner 0.1.7 explicitly still reports Alexa GS014. Its log again reaches CASE
  and CommissioningComplete persistence, then UnsupportedEndpoint; command and
  cluster rejection probes precede completion too. Allocation failures remain
  zero; UDP records four temporary retries with no hard errors/timeouts, heap
  minimum 3476 and foreground watermark 692 bytes. The sorted-metadata fix is
  valid but does not establish the Alexa failure cause. Do not equate any single
  Unsupported* label with a fatal pairing error or claim safe memory margins.
- 0.1.8 adds numeric rejection paths and the actual sorted endpoint inventory at
  startup. Parse only pinned typed AttrStatus/AttrPath and CmdStatus/CmdPath
  formats, forwarding endpoint (u16), cluster/leaf (u32) and operation 1=read,
  2=write, 3=command. 0xffffffff represents a missing/wildcard field. Reject
  malformed/overflowing fields; never forward node/fabric IDs, names, credentials,
  certificates, bodies or original SDK text. Keep the 256-byte formatter and
  32-event queue, with <=16 bytes per numeric event and explicit overflow.
  Drain inventory and paths outside stack/queue locks through ROM output.
  No SDK fork, task-stack/protocol-capacity reduction, identity reset, protocol workaround or
  promised GS014 fix. Exact request paths are required before deciding whether
  the post-completion rejection is causal or a harmless controller probe.
- Rebalance the existing 32 KiB foreground/future reservation: the ESP32 future
  arena is 14 KiB and the foreground CPU task stack 18 KiB (formerly 16+16).
  The operational future still needs 12664 bytes; retain the exact-type compile
  assertion for size + alignment + 1024 reserve. Host arena stays 24 KiB. This
  increases task-stack headroom for observed commissioning depth without growing
  these two reservations' sum or changing protocol buffer/peer/station limits.
  Diagnostic numeric storage has its own small static RAM cost. Log the configured
  loop_stack_bytes; no safe hardware stack/RAM margin or GS014 fix is established.
- Direct LAN access to deployed 0.1.8 is now available through the owner's
  personal web session. Its four stations expose actual Matter endpoints 3..6,
  root PartsList [2,3,4,5,6], aggregator-2 PartsList [3,4,5,6]. A real encrypted
  PASE Descriptor read returns UnsupportedEndpoint at 1, superseding the owner's
  earlier manually reported boot inventory including 1. No station assignments
  changed. The probe closes its session, sends no NOC/fabric/power/firmware changes,
  and leaves readiness, pairing, command histories and uptime intact. Do not put
  the address, login, cookies, API key, setup code, MACs or raw private responses
  into source, fixtures, documentation or release artifacts.
- The matter.js ECOSYSTEMS.md primary documentation records Alexa's non-standard
  endpoint-1 requirement (main device or aggregator). A missing-1 production
  runtime regression fails before the candidate fix with the exact hardware
  1/29/0 UnsupportedEndpoint response. Earlier tests included a light at 1 and
  did not cover the deployed topology. This is an established incompatibility,
  subsequently followed by owner-confirmed successful Alexa pairing on 0.1.9.
- Candidate 0.1.9 adds Descriptor-only aggregator 1 only if no active station
  owns 1. Keep aggregator 2 as an empty retained endpoint in this case; all
  station IDs, UniqueIDs, names, bindings and fabric storage stay unchanged.
  If a legacy station owns 1, keep it a light and aggregator 2 as its parent.
  Custom PartsMatcher puts each station under exactly one aggregator, excluding
  both aggregator endpoints from child lists. Allocate metadata once for the
  actual station count plus 2/3 endpoints; up to 19 endpoints (16 stations).
  Increase only the bounded startup inventory from 18 to 19 IDs, not capacities
  for stations, sessions, subscriptions, task stacks or protocol packets.
- The owner explicitly approved deployment of 0.1.9 and assigning currently
  unbound 1 an aggregator role as a narrow exception to the previous retired
  endpoint rule because the old registry cannot prove whether 1 was ever used.
  Never recycle retired station bindings or renumber an active light for this.
  Keep aggregator 2 and all registry/NVS data. This exception is not permission
  for future device changes or additional endpoint migrations.
- Host tests exercise encrypted established CASE IM traffic through the running
  production runtime/UDP FFI, actual parsed device types/root/aggregator/leaf
  PartsList/light ServerList, reversed/repeated/batched reads, unsupported-path
  isolation, scalar ReportData chunking/ACKs, exact rejection paths and no power
  writes. Isolate singleton runtime fixtures in child test processes, covering
  legacy 1, five stations, missing 1 with four/sixteen stations, and zero/sparse
  configurations. These seed synthetic public-API fabric/ACL/session state;
  These host fixtures do not test real CASE establishment. Actual OTA, real
  PASE/Descriptor verification and owner-confirmed Alexa pairing are established
  separately below.
- Candidate Descriptor DataVersions are randomized once per boot, not fixed at
  1, so previously cached versions cannot hide the changed retained PartsList.
  Local candidate verification: eight Rust tests in debug/release, sixteen native
  C++ programs, 31 Python checks and nine browser/USB simulations pass. App
  1,925,280/1,966,080 bytes, static RAM 67,524 bytes; exact slots/chip/OTA/hash/libc
  checks pass. ELF confirms arena 14 KiB, Stack 52,528 bytes, CPU task 18 KiB;
  exact future + alignment + 1 KiB guard passes. These software checks do not
  establish successful Alexa pairing or safe runtime margins.
- Owner-approved physical OTA from 0.1.8 to 0.1.9 succeeded through the personal
  session/token/inactive-slot path. The device rebooted into 0.1.9 with Matter
  ready/paired and BLE worker available, no startup error; all four assignments,
  API key, web password and device naming matched the pre-update snapshot.
  Command histories remained none. One post-boot sample was uptime 4337 ms,
  free byte heap 26584/largest 25588: this is not a runtime margin guarantee.
  The diagnostic web session was closed. The owner subsequently explicitly
  authorized opening a fresh 15-minute pairing window, direct endpoint inspection
  and returning its setup code privately. A real encrypted PASE probe on 0.1.9
  confirms root PartsList [1,2,3,4,5,6], aggregator 1 type 0x0e/revision 2 with
  PartsList [3,4,5,6], retained aggregator 2 with empty PartsList, and unchanged
  light/bridged types on 3..6. All reads succeed; no UnsupportedEndpoint, reboot
  or power command occurs. The probe closes its PASE and web sessions while
  leaving the owner's approved window open with the same code. Never commit
  that code. The owner subsequently confirms complete Alexa pairing on 0.1.9.
  This is four-station owner hardware evidence, not sixteen-station/long-run
  validation or proof of every synchronization path.
- The owner explicitly requested a web-interface all-off synchronization test.
  A real headless Chromium session logged into the deployed dashboard and clicked
  its actual All to SLEEP button once. Before: all four local IDs 1..4 measured
  steady ON/raw 11 and reachable. After: all four measured SLEEP/raw 0, reachable,
  command_status=confirmed, pending=false/busy=false. The browser session signed
  out. No configuration/firmware/fabric changes or additional power commands were
  made. This proves the real UI-to-BLE transition; Alexa app state synchronization
  for that transition still needs the owner's observation.
