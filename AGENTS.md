# Lighthouse Bridge — project instructions

Read this file before edits. **Update it in the same change** when requirements,
hardware, architecture, languages, auth, flashing, workflows or verification
status change. Keep current facts, not a conversation log. User instructions win.

## Purpose and hardware

- Repository `Traumlewis/Lighthouse-Bridge`, authorized working branch `main`.
- Classic dual-core ESP-WROOM-32 / ESP-32S, CH340 USB-C, at least 4 MB flash.
  PlatformIO `esp32dev`. Never substitute S2/S3/C3 firmware.
- Up to **16 Valve Lighthouse V2**, controlled by **Alexa**. The owner currently
  has four physical stations; this is test evidence, not a capacity/model restriction.
- **ON = `0x01`; OFF means SLEEP = `0x00`. Never send STANDBY (`0x02`).**
  STANDBY may only appear as a measured state.
- No extra phone companion app. General local HTTP API for integrations. Do not
  introduce smartwatch-specific plans. Local firmware upload is implemented in
  0.3.0; Improv Serial is not implemented.
- Never send power commands automatically on boot/reboot/Wi-Fi reconnect. Pure
  BLE reads at startup and periodically are intended and required.

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
  discovery. Results contain const device pointers; copy data before clearing.
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

- Experimental **0.2.0-matter** is a separate, multi-station Matter prototype in
  `experimental/matter/MatterBridge/`, offered at the existing Pages `matter.html`.
  Stable firmware stays **0.3.9** with its existing PlatformIO environment. The
  owner explicitly rejected larger/single app slots: **retain both 1.875 MiB slots
  and the exact complete partition table for the prototype too**. NimBLE makes the
  size feasible; the minimal Bluedroid probes exceeded the slot. Never package an
  oversized build or substitute S2/S3/C3 firmware.
- The isolated Arduino CLI build pins CLI 1.6.0, Arduino ESP32 3.3.12,
  NimBLE-Arduino 2.5.2 and ArduinoJson 6.21.5. CHIPoBLE is compile-time disabled;
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
- Prototype scope is all 0–16 configured stations with nonempty MACs, read through
  the shared parser from the stable NVS record without rewriting it. It has no
  dashboard, local HTTP API, station editor, discovery UI or OTA upload. Set up
  stable Wi-Fi/assignments first. Install/return by USB with erasing unchecked;
  existing Hue entries are unavailable while the prototype runs. Its per-board
  setup credentials live under `lhmatter`; Matter also persists fabric data.
  Test bridge name is exactly **Lighthouse-Bridge Matter Test**; the owner wants
  **Lighthouse-Bridge** after the test phase. Never rename saved station names.
- `include/matter_config.h` owns the persistent endpoint registry under
  `lhmatter/endpoints`. Bind by full stable Hue identity + MAC, not local slot or
  array position. Migrate the former first station to endpoint 1 and retain
  aggregator 2; allocate other endpoint IDs monotonically from 3, never reuse a
  retired endpoint. Validate the registry; fail closed on corruption/exhaustion.
  Checkpoint current+proposed bindings (up to 32) so interrupted saves cannot
  renumber survivors; current active capacity remains 16. No silent truncation.
- The Matter build generates `shared_ble_worker.h` from the production worker
  section of `src/main.cpp`; never maintain a second retry/confirmation loop.
  All BLE work remains sequential. USB `LH_MATTER_INFO` adds a validated list of
  measured stations and `bridge_name`, preserving the legacy first-station fields.
  Show actual fixed local IDs and saved names; changes of language retain USB.
- `tools/build_matter.py` writes split images/checksums/ZIP to `dist/matter/`;
  Pages validates exact partition-table equality, slot bounds, chip IDs and hashes.
  No prototype merged image or `.lhota`. `site/matter.js` / `matter-info.mjs` use
  `LH_MATTER_INFO` through the shared serial watcher; pairing codes are physical
  USB + page memory only, never URLs/storage/network. Preserve one serial owner,
  EN/DE edits/connections and explicit experimental consent.
- No hardware is attached to the development environment. The owner's initial
  0.1.0 result supports basic operation; runtime memory, persistence/return across
  framework versions, prolonged BLE coexistence and exact Alexa status behavior
  remain unverified. The 0.1.1 lamp/switch/status report is positive owner evidence;
  multi-station 0.2.0 still needs hardware testing before the next phase.
  Test attestation is not product certification. Do not claim a fix from a build
  or simulated browser test. Keep the prototype guide and validation report current.
- The owner wants staged work: single-station status first, multiple Matter lamps
  next, then dashboard/API/Wi-Fi updates. Stage 1 is owner-confirmed; 0.2.0 is the
  next multi-station test. Stage 3 is not shipped. An early full-services size
  probe with gzip UI still exceeded the slot (1,990,075 bytes, or 1,970,027 with
  WiFiManager debug compiled out); more optimization is required. Keep slots fixed.

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
python tools/build_matter.py
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
