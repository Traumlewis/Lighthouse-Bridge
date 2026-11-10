# Lighthouse Bridge — project instructions

Read this file before edits. **Update it in the same change** when requirements,
hardware, architecture, languages, auth, flashing, workflows or verification
status change. Keep current facts, not a conversation log. User instructions win.

## Purpose and hardware

- Repository `Traumlewis/Lighthouse-Bridge`, authorized working branch `main`.
- Classic dual-core ESP-WROOM-32 / ESP-32S, CH340 USB-C, at least 4 MB flash.
  PlatformIO `esp32dev`. Never substitute S2/S3/C3 firmware.
- Four **Valve Lighthouse V2**, Echo Dot **5th generation**.
- **ON = `0x01`; OFF means SLEEP = `0x00`. Never send STANDBY (`0x02`).**
  STANDBY may only appear as a measured state.
- No extra phone companion app. Local HTTP API is intended for later Pebble
  integration; no Pebble plugin, OTA updater or Improv Serial implementation yet.
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
  Preserve BLE address types and reject duplicate station MACs/names.
- `requested` is only a goal. **Dashboard and local API always use `observed`
  measurements**, with timestamps/reachability. The owner explicitly approved a
  Hue-only target window after the 0.2.5 Echo retest: while an accepted command
  is pending/busy, Hue `on` follows the target for at most 30000 ms from acceptance.
  Completion (including unconfirmed writes), final error or expiry restores the
  measured `on` immediately. Never extend the window on duplicate active requests.
  `reachable` stays measured throughout; do not force it true or modify observed
  values/timestamps. No window exists after reboot or without a command.
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
  assigned MAC, require connectable advertising and use its current address type
  only for that job. Do not persist changes or accept a device by name alone.
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
  this grace to an unconfirmed post-command state. Expose read_failures, ble_error
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
- Espalexa exposes four lights plus **`VR Basisstationen`**. Preserve existing
  light IDs and that group name. Group ON aggregates the individual Hue views
  (bounded active targets, otherwise measurements); group reachability requires
  fresh measurements for all assigned stations. Mixed targets remain consistent.
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
  Preserve encoded IDs, names and group membership; Espalexa still handles SSDP,
  description and username registration, not light PUTs or measured light GETs.
- Serial `Hue PUT` / `Hue GET` diagnostics include station (0=group), target or
  measured value, acceptance/reachability and pending work. Collection GETs log
  configured/fresh counts. Log no usernames, raw URLs, bodies or access keys.
  GET source=accepted_target or source=measurement identifies the selected Hue
  view (group source means at least one target contributed). PUT handler_ms
  measures the handler only, not network/end-to-end latency.
- **Local Hue emulation cannot push status into the Alexa app.** Alexa controls
  its own polling/cache. Do not promise immediate app-state synchronization.
- Keep Wi-Fi modem sleep enabled (`WIFI_PS_MIN_MODEM`) before BLE starts and
  after reconnect. Set Arduino's cached policy before STA starts, and check the
  driver result. `WiFi.setSleep(false)` / `WIFI_PS_NONE` caused the 0.1.0 startup
  abort in `coex_enable()`. This setting is unrelated to Lighthouse SLEEP.

## Wi-Fi, passwords and USB

- WiFiManager uses a **non-blocking** protected setup portal so USB info can be
  serviced before Wi-Fi is configured. Normal HTTP starts after connection and
  BLE initialization. BOOT held for 5 seconds opens setup when BLE is idle.
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
  access to `/api/account` or `/api/password`. Those require a web session.
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

## Critical flash-layout rule

- Current release **0.2.6**. Web updates use exactly four files at offsets:
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

## Languages and source layout

- **Keep the GitHub repository in English:** README, project documentation,
  authored comments, non-localized diagnostics, commit messages and generated
  release instructions. Do not add German-only repository documentation.
  German belongs in intentional UI translations and tests of those translations.
  Preserve upstream licenses and established identifiers such as
  `VR Basisstationen`; do not rename user/device data for language consistency.
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
python tools/build_pages.py
python -m unittest discover -s test -p 'test_pages.py' -v
node test/serial_info.cjs
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
view and coalesces identical in-flight requests. Its effect on the actual spoken
warning and remaining BLE reliability still need a real Echo/board retest.
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
