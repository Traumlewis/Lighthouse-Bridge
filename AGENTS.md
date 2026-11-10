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
  Old results must not clear newer requests. Preserve BLE address types and
  reject duplicate station MACs/names.
- `requested` is only a goal. **Dashboard and Hue GETs use the same `observed`
  measurement**, not the last button pressed. Expose timestamps and reachability.
- Read on startup, after commands, and about every 15 seconds; booting states
  retry after 2 seconds, unreachable devices after 30 seconds. One radio means
  queue/connection delays can lengthen intervals. Manual refresh is read-only.
- Read-only queries use a 6-second connection timeout and up to two attempts,
  with 350–650 ms between attempts. Yield to queued commands/reassignments before
  and after retry scanning; command connection timeout stays 3 seconds. Allow
  150 ms between jobs.
- BLE code 574 = 0x023e = HCI 0x3e, connection establishment failed. On this
  error only, scan passively for 2 seconds before the next attempt. Match the
  assigned MAC, require connectable advertising and use its current address type
  only for that job. Do not persist changes or accept a device by name alone.
  If unseen, retry the saved address anyway (a short scan is not proof of absence).
  Log the actual phase/code/attempt and recovery RSSI/type or lack of advertising.
  Recovery scans never write power and do not replace the user discovery results.
- One failed read cycle retains a previously usable measurement, explicitly labeled
  last reading / retrying, without changing its timestamp. Retry after 5 seconds.
  Two consecutive failed cycles, no prior reading, or age >=60 seconds means
  unreachable. Counters saturate and reset on a successful reading. Never grant
  this grace to an unconfirmed post-command state. Expose read_failures, ble_error
  and last_read_attempt_ms in status; log numeric BLE codes without device keys.
- Reassignment may proceed during a background read. Check its generation before
  applying the result; never apply an old device reading to a new assignment.
  A separate physical BLE-active flag must still block starting the Wi-Fi portal.
- Readings older than 60 seconds are stale. Raw 1/8/9 = booting, 11 = ON, 0 = SLEEP, 2 = STANDBY.
  Booting counts active for Hue; dashboard still says Starting.
- Espalexa exposes four lights plus **`VR Basisstationen`**. Preserve existing
  light IDs and that group name. Group ON/reachable requires all assigned stations
  active/fresh. Hue GET responses include measured `on` and `reachable`.
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

- Current release **0.2.2**. Web updates use exactly four files at offsets:
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

Owner reports **0.1.1 starts and basic operation works**, but settings were lost
on update and status was not synchronized. The 0.2.0 extensions have passed local
build/model/browser checks; **the owner reports intermittent BLE connection failures also without another
controller. The owner subsequently reports repeated code 574 on all four stations.
Version 0.2.2 adds scan-assisted recovery and precise phase/attempt diagnostics;
the radio cause and effectiveness still require an actual hardware retest**.
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
