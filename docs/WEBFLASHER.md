# GitHub Pages / WebFlasher

**https://traumlewis.github.io/Lighthouse-Bridge/**

ESP Web Tools 10.4.0 is vendored unchanged under `site/vendor/`, including its
lazy-loaded modules and published integrity values in `UPSTREAM.json`. No runtime
CDN is required. GitHub Pages provides HTTPS; `http://localhost` also works.
Use Chrome or Edge on a computer.

## Experimental Matter choice

`matter.html` is a separate, explicitly experimental installer for
`0.3.0-matter`; the main page and `manifest.json` continue to offer stable 0.3.9.
The full test firmware retains saved settings and includes the production
dashboard, discovery/editor, local API and Wi-Fi updates for up to 16 stations.
New installations have a protected non-blocking Wi-Fi setup portal. The USB panel
reads key/address using `LH_INFO` and pairing/status using `LH_MATTER_INFO` through
one serial connection. After personal-password login, **Matter setup** in the
local dashboard reveals the code and can open a 15-minute pairing window.
Keep erasing unchecked to retain settings when installing or returning to stable.
See the [prototype guide](../experimental/matter/README.md) for pairing and limits.

`tools/build_matter.py` builds with the pinned Arduino CLI environment and packages
`dist/matter/`. `tools/build_pages.py` verifies its four segments, classic ESP32
headers, size limit, per-image hashes and exact equality of both partition tables,
then creates `_site/matter-manifest.json` and `firmware/matter/`. Both app slots and
the NVS addresses are retained. No merged factory image is offered for
the experiment. A separate `firmware/matter/lighthouse-bridge-0.3.0-matter.lhota`
contains only the app for subsequent dashboard uploads; Pages checks it against
the USB app image. Its ZIP includes source, checksums, version notes and licenses.

After the installer releases USB, the experiment page sends only
`LH_INFO\nLH_MATTER_INFO\n` through the shared serial watcher. Both validated
parsers expose the setup key/address and the Matter code, bridge name and measured
station list. Keys and codes remain in browser memory, never URLs/storage/network. EN/DE changes preserve
the connection and consent checkbox. Firmware installation stays disabled while
the reader owns USB and when its manifest or any segment is unavailable/unsafe.

## Languages

Repository documentation is English. The page defaults to English and offers
German in its language selector. It stores the choice under
`lighthouse-bridge.language` in localStorage. `?lang=en` or `?lang=de` overrides
that preference; otherwise it defaults to English. Switching still works if
browser storage is blocked. `site/translations.js` contains page copy, statuses,
errors, metadata and labels. Language changes preserve checkboxes and USB connections.

The ESP32 dashboard uses the same English/German selection rules. The unmodified
ESP Web Tools dialog and browser USB picker have their own language behavior and
may remain in English.

## Settings-preserving updates

**Since 0.2.0 the installer uses four separate images:**

| File | Offset |
| --- | --- |
| `lighthouse-bridge-bootloader.bin` | `0x1000` |
| `lighthouse-bridge-partitions.bin` | `0x8000` |
| `lighthouse-bridge-boot-app0.bin` | `0xe000` |
| `lighthouse-bridge-app.bin` | `0x10000` |

The NVS region `0x9000..0xdfff` is neither written nor erased by adjacent sectors.
**Leave Erase device unchecked** for updates. `new_install_prompt_erase: true`
provides that choice.

**The 0.1.x bug:** Writing the old merged image at `0x0` overwrote NVS with padding.
The earlier advice to deselect erase was insufficient for that image. The merged
factory BIN remains available for fresh installations and is labeled accordingly.
Use the WebFlasher or four separate images at their respective offsets for updates.
The partition layout remains unchanged.

`tools/package_release.py` creates and cross-checks the factory and split images.
`tools/build_pages.py` copies them to `_site/`, generates the four-part manifest
and verifies that their sector ranges exclude NVS. `test/test_pages.py` simulates
sector erases/writes with existing NVS content. `site/flash-layout.mjs` rejects
unsafe manifests before enabling the installation button.

Each part URL includes its SHA-256 checksum as a query parameter so the browser
cannot reuse an older cached image on the next update.

## USB key and Wi-Fi address display

1. Close the ESP Web Tools dialog after installation.
2. The page reuses its granted port and starts the USB information reader.
   Alternatively, select **Read key & address** and choose the port.
3. The API key and setup Wi-Fi name appear when firmware responds. Keep USB
   connected; the local web address appears after Wi-Fi setup completes.
4. Select **Disconnect USB** before flashing again. A serial port can have only
   one reader; close other consoles/applications using it.

Current firmware implements `LH_INFO\n` at 115200 baud. While connected,
`site/serial-info.mjs` queries every two seconds. Allow up to 45 seconds for the
first response after boot/connection attempts. Firmware also processes USB
commands while the non-blocking setup portal is active. Older firmware without
this protocol must be updated first.

Addresses are validated: only a plain HTTP IPv4 address with path `/` is offered
as a link. Keys are not stored in localStorage, cookies or URLs, or sent to GitHub.
The personal web password is never exposed in USB information or the installer.

ESP Web Tools owns the port until its dialog closes. The page waits for its
`closed` event before taking over and never interrupts flashing. The USB parser
handles fragmented lines and ignores other logs and invalid frames. Improv
Serial is **not implemented**; Wi-Fi setup uses the ESP32 portal rather than the
ESP Web Tools provisioning dialog.

## Build and deployment

Pages already uses **GitHub Actions**. A push to `main` runs native tests, builds
firmware, packages images, checks page/NVS and USB behavior, and deploys on success.
Pull requests are checked without deployment. Artifacts are
`lighthouse-bridge-firmware` and `lighthouse-bridge-debug` (ELF/map symbols).

```sh
pio run
python tools/package_release.py
python tools/build_matter.py
python tools/build_pages.py
python -m unittest discover -s test -p 'test_pages.py' -v
node test/serial_info.cjs
npm test
python -m http.server 8080 --directory _site
```

`site/` contains source; `_site/` is generated and ignored. All paths must work
under `/Lighthouse-Bridge/`. Browser tests simulate devices and do not prove a
real flash operation. Keep version references in `src/main.cpp`, `include/web_ui.h`
and `tools/package_release.py` aligned with documentation and `AGENTS.md` when
making an actual firmware release. Documentation translations do not change the
firmware's functional version.

## Wi-Fi update packages and key copying

The page also offers `firmware/lighthouse-bridge-v<VERSION>.lhota`. Install >=0.3.0
once over USB; later download the package and upload it through the device's
**Firmware update over Wi-Fi** section. This uses the inactive application slot;
NVS and partition locations remain unchanged. The .lhota file is separate from
the four-part Web Serial manifest. It is also in the ZIP and SHA256SUMS.
`tools/ota_package.py` validates the classic ESP32 chip, app descriptor, size and
product marker, and wraps the app with its length and SHA-256 digest. Pages build
checks that the packaged payload equals the USB application image. It is an
integrity container, not a signed release or automatic crash rollback system.

Click the displayed key to copy it. The dashboard supports the same action on
local HTTP using a selection/copy fallback when Clipboard API is unavailable.
Neither copy path stores the key or sends it to another service. Keyboard activation
works through native buttons. The Wi-Fi portal itself uses dashboard styling;
a custom device name controls both setup SSID and the .local address.
