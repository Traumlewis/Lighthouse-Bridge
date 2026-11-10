# Lighthouse Bridge — project instructions

Read this file before changing the project. Keep it current: update it in the
same change whenever requirements, hardware, language behavior, architecture,
build/deployment steps or verification status change. Record current facts and
decisions, not a conversation log. User instructions take precedence.

## Purpose and fixed requirements

- Repository: `Traumlewis/Lighthouse-Bridge`; working branch: `main`.
- Device: classic dual-core ESP-WROOM-32 / ESP-32S development board, CH340 USB-C,
  at least 4 MB flash. PlatformIO board `esp32dev`. Do not substitute S2/S3/C3.
- Control **four Valve Lighthouse V2** base stations over BLE, sequentially.
- **ON = `0x01`; OFF means SLEEP = `0x00`. Never send STANDBY (`0x02`).**
  A read-only STANDBY observation may be displayed, but never offered as a command.
- Alexa target: **Echo Dot, 5th generation**. Espalexa emulates four on/off lights
  and the group light **`VR Basisstationen`**. Alexa state is optimistic, not a
  confirmed BLE reading. Keep that group name stable unless the user asks otherwise.
- No extra phone companion app. An ESP32 HTTP API is reserved for future Pebble
  integration; the Pebble plugin has not been implemented yet.
- First-boot Wi-Fi setup uses a password-protected ESP32 access point and captive
  portal. Keep the mini setup interface and the BOOT-button recovery path.
- Never automatically switch stations on boot, reboot or Wi-Fi reconnect.

## Languages and communication

- Communicate with the owner in German unless they ask otherwise.
- Public WebFlasher: **English by default; German as a selectable second language**.
- `?lang=en` / `?lang=de` override a saved preference. Otherwise use
  `localStorage['lighthouse-bridge.language']`, then English. Do not select German
  automatically from the browser locale. Storage failure must not break switching.
- Translate the full page, status/errors, help, metadata and accessible labels.
  Keep `site/translations.js` keys complete in both languages and the initial HTML
  usable in English. Only locally authored translations may contain markup.
- Language switching must preserve the board checkbox, installer controls and
  open help sections. Never initiate USB access or flashing on a language change.
- ESP Web Tools and the browser USB picker retain their upstream language behavior;
  the page explains this. Do not edit vendored modules to translate their dialogs.
- The firmware's embedded local web UI remains German. Its translation is a
  separate change, not implied by changes to the public WebFlasher.

## Architecture and important files

- `src/main.cpp`: Wi-Fi provisioning, NVS, BLE worker, HTTP/API and Espalexa.
- `include/bridge_core.h`: ON/SLEEP mapping, input validation, request generations.
- `include/web_ui.h`: local ESP32 dashboard embedded in firmware.
- `site/`: public GitHub Pages source; `index.html`, `style.css`, `app.js`,
  `translations.js`. All asset/manifest URLs must work below `/Lighthouse-Bridge/`.
- `site/vendor/`: unmodified ESP Web Tools **10.4.0**, including lazy-loaded
  modules and `UPSTREAM.json` integrity hashes. No runtime CDN dependency.
- `tools/package_release.py`: version **0.1.1**, merged image and source ZIP.
- `tools/build_pages.py`: validates vendor/firmware integrity and creates `_site/`.
- `.github/workflows/firmware-pages.yml`: build, checks, artifacts and Pages deploy.
- `README.md`, `docs/API.md`, `docs/WEBFLASHER.md`, `docs/VALIDATION.md`: keep relevant
  user and developer instructions consistent with the implementation.

## Firmware details to preserve

- BLE service: `00001523-1212-efde-1523-785feabcd124`.
- Power characteristic: `00001525-1212-efde-1523-785feabcd124`.
- Up to three connection/write attempts; newer pending requests replace older
  ones for the same station. Old results must not clear a newer request.
- One BLE task owns scan/connect/write/disconnect; HTTP and SSDP remain responsive.
- Keep **Wi-Fi modem sleep enabled (`WIFI_PS_MIN_MODEM`) before initializing BLE**.
  With Arduino ESP32 2.0.17, disabling it (`WiFi.setSleep(false)` / `WIFI_PS_NONE`)
  aborts inside `coex_enable()` when the BT controller starts. Do not disable it
  later to improve HTTP/SSDP latency. This modem setting does not change the
  Lighthouse SLEEP command or put the ESP32 CPU into deep sleep.
  Set Arduino's cached mode before starting STA as well as checking the driver
  setting before BLE; STA restarts can reapply Arduino's cached policy.
- Read state once after a command when available. Display reading age; do not
  present it as continuous monitoring or treat `0x01` as confirmed ON.
- Preserve BLE address type from discovery and reject duplicate assignments.
- Web/API authentication: `admin` plus device-generated key, or `X-API-Key`.
  The same 24-character key protects setup Wi-Fi and is printed over USB serial.
  Never commit actual keys, SSIDs/passwords or user-specific station MACs.
- Hue endpoints are intentionally unauthenticated for Alexa on the trusted LAN.
- Firmware updates currently use USB. OTA and Improv Serial are **not implemented**.
- Merged ESP32 image flashes at **`0x0`**, with DIO, 40 MHz, 4 MB layout. Never
  offer an application-only image at that offset. Offer the erase choice; updates
  without erase should preserve NVS, Wi-Fi, key and station assignments.

## Validation

Use checks appropriate to the change; a website-only change does not require
altering or repeatedly rebuilding firmware locally. CI builds it before deploy.

```sh
# Firmware and request logic
pio run
g++ -std=c++11 -Wall -Wextra -Werror -I include test/test_core.cpp -o /tmp/lighthouse-test
/tmp/lighthouse-test

# Prepare and verify the published assets after a firmware build
python tools/package_release.py
python tools/build_pages.py
python -m unittest discover -s test -p 'test_pages.py' -v

# Browser checks (npm dependencies + Chromium required)
node test/web_flasher.cjs
node test/web_ui.cjs
```

`PLATFORMIO_CORE_DIR` or `--pio-core-dir` on the packaging script selects a custom
PlatformIO installation. Browser tests use `CHROMIUM_PATH` when provided. Serial
access is stubbed in the WebFlasher test: never flash connected hardware merely
to test page rendering. For language changes run the static translation checks
and the bilingual browser test. Keep generated `_site/`, `dist/`, `.pio/` and
`node_modules/` out of Git; edit source files, not generated output.

Current verification: native logic, page asset/integrity and EN/DE browser checks
pass; GitHub Pages is enabled. On 2026-11-10 the owner reported a real 0.1.0 boot:
firmware starts and connects to Wi-Fi, then aborts during BT-controller coexistence
initialization. Version 0.1.1 corrects the incompatible Wi-Fi power-save setting;
the owner must still confirm startup after reflashing. **Lighthouse BLE operations,
full Wi-Fi-portal recovery and Echo Dot 5 control remain unverified on hardware.**
Keep hardware support labeled prototype and record new evidence here and in
`docs/VALIDATION.md`. CI retains matching ELF/map files as the
`lighthouse-bridge-debug` artifact for future crash decoding. Never commit raw
serial logs containing access keys or personal network/device identifiers.

## Repository and publication workflow

- Inspect the current remote head and existing instructions before edits; preserve
  owner changes and the existing MIT license. Do not force-push.
- Commit completed, checked changes to the authorized repository. Pushes on `main`
  automatically build and publish; pull requests build without deployment.
- GitHub Pages URL: **https://traumlewis.github.io/Lighthouse-Bridge/**.
- Pages source is **GitHub Actions**. Do not create a separate hosted Site or
  change repository visibility. Use the existing workflow.
- Follow a publication through its terminal result. If only deployment failed,
  rerun failed jobs after resolving the cause; do not rebuild successful jobs
  unnecessarily. Inspect actual logs on failure.
- After a website change, confirm the deployed content/language behavior. Verify
  the published manifest/image checksum when firmware or packaging changes.
- Keep this file and the relevant docs aligned with each completed change; state
  clearly what was tested locally, in CI, on the live site or on real hardware.
