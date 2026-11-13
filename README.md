# Lighthouse Bridge — ESP32

Control **up to 16 Valve Lighthouse V2 base stations** using **Alexa**, a local
web interface or the HTTP API. Runs on the classic **ESP-WROOM-32 / ESP-32S
CH340 USB-C board**, with at least **4 MB flash** and 2.4 GHz Wi-Fi. No extra
phone companion app is required. Not for ESP32-S2/S3/C3.

**ON = `0x01`. Off = SLEEP (`0x00`). Never sends STANDBY (`0x02`).**
Startup, reboot and Wi-Fi reconnection never send power commands automatically.

**[WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/)** ·
[German UI](https://traumlewis.github.io/Lighthouse-Bridge/?lang=de) ·
[Builds](https://github.com/Traumlewis/Lighthouse-Bridge/actions) ·
[API reference](docs/API.md) · [Validation](docs/VALIDATION.md)

Repository documentation is English. The installer and dashboard default to
English and offer German; language changes preserve edits and connections.

## Features

- Up to 16 stations with fixed IDs **1–16**. New entries use the lowest free ID;
  deleting or reordering leaves the other IDs unchanged.
- Individual Alexa devices, local web controls and an authenticated HTTP API.
- Bluetooth discovery and a single address field; Random (1) is used automatically.
- English/German dashboard, a custom setup Wi-Fi name and matching `.local` address.
- Separate web password and integration key, with click-to-copy key display.
- Firmware updates over Wi-Fi using `.lhota` packages.

Detailed status diagnostics are available by triple-clicking the firmware version.
Repeat to turn Debug off. Cards keep their size; long diagnostics scroll inside.

See [VERSION.md](VERSION.md) for the version history and [validation](docs/VALIDATION.md)
for software checks and hardware evidence.

A separate [rs-matter prototype](experimental/rs-matter/README.md) is available
through its [own WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/rs-matter.html).
It combines the independent Rust rs-matter implementation with the complete Lighthouse dashboard,
authentication, API, BLE worker and OTA receiver. The original app slots/NVS and
station/endpoint identities are retained. Changing Matter engines requires a new
pairing; the previous fabric data stays stored for return. Actual RAM headroom,
Alexa pairing, multi-station operation and OTA still need hardware testing.

An isolated [experimental Matter prototype](experimental/matter/README.md) is
available through the [Matter test WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/matter.html).
It provides **up to 16 separate Matter lights**, the dashboard, discovery/station
editor, local API and Wi-Fi firmware updates on the same classic ESP32 and exact
app-slot/NVS layout. Retrieve the setup code in the dashboard's **Matter setup**
section after signing in with your personal password; open its pairing window
there when needed. Install the separate Matter `.lhota` for later updates, or
return to stable over USB. Keep erasing unchecked to preserve settings.
The owner confirms ON/OFF, lamp classification and status for the earlier
single-station test. Full integration and multi-station operation still need
hardware testing. The test bridge is **Lighthouse-Bridge Matter Test**;
**Lighthouse-Bridge** is planned after testing.

## First installation and upgrade

1. Open the WebFlasher in Chrome/Edge on a computer and connect the supported
   ESP32 with a USB data cable. Confirm the board and choose **Connect & install**.
2. For an existing installation, **leave Erase device unchecked** to preserve
   Wi-Fi, assignments, passwords and keys. The installer writes four separate
   flash regions, leaving NVS untouched. The downloadable merged factory BIN
   resets settings even without a full erase; it is only for a fresh installation.
3. Close the installer. Its USB panel displays the API key and setup Wi-Fi name.
   Alternatively choose **Read key & address**. Click the key to copy it.
4. On first setup, connect to `Lighthouse-Setup-XXXXXX` with that key as password.
   Open `http://192.168.4.1`, choose your home Wi-Fi and save. The USB panel shows
   the device's local address once connected.
5. Open the address from your home network. First login: **admin + API key**.
   Then choose a personal web password (8–128 bytes, different from the key).
   Later logins use that password. The API key stays visible under **Wi-Fi & local API**.
6. Under **Set up base stations**, discover `LHB-…` devices. Add an entry for each
   station, choose/enter its address and give it a unique name, then save.
   An empty address disables an entry; Remove deletes it on the next save.
7. Keep Alexa and ESP32 on the same home network, without client isolation.
   Ask Alexa to discover devices. Test one station, then collective ON/SLEEP.

No USB port? Check the data cable/CH340 driver and close other serial applications.
If flashing stalls: hold BOOT, briefly press EN/RESET, release BOOT when connected.
Normal restart: EN/RESET only. Serial console speed is 115200 baud.

## Wi-Fi firmware updates (OTA)

Install **0.3.0 or later once over USB** to enable the upload interface. Afterwards:

1. Download the **Wi-Fi update package (.lhota)** from the WebFlasher page.
2. Sign in to the device with your personal web password. Open **Firmware update
   over Wi-Fi**, select the package and choose **Install update**.
3. The bridge waits for a current background read, pauses BLE, validates and
   writes the inactive app slot, then restarts. Keep power connected until restart.
   Sign in again afterwards; settings remain in NVS.

Only official Lighthouse Bridge packages for classic ESP32 are accepted. Raw app
BINs, merged factory BINs and ZIPs are not upload packages. The package includes
product/chip checks, payload length and SHA-256 integrity; it is **not a digital
signature**. Use files from a trusted build. Interrupted/invalid uploads do not
select the new boot slot. This is not an automatic crash rollback after a
successfully installed image. USB recovery remains available.

## Actual state, commands and Alexa

| Raw read | Dashboard | Measured Hue `on` |
| --- | --- | --- |
| `0` | SLEEP | false |
| `1`, `8`, `9` | Starting | true |
| `11` | ON | true |
| `2` | STANDBY (read only) | false |
| No usable read | Unknown | last known state, unreachable |

The dashboard/API always use real measurements. A requested target is separate.
One worker serializes BLE operations. Reads run at startup, after commands and
periodically (target 15 s; actual intervals grow with station count/retries).
A recent previous reading survives one failed cycle with a warning; repeated
failures or age >=60 s means unavailable. Failed reads never refresh its timestamp.

A successful write is **sent**, not necessarily **confirmed**. Read-only checks
about every two seconds for up to 30 seconds confirm steady ON (11) or SLEEP (0).
Booting values do not confirm steady ON. The last-command result remains history;
current state can change later through another controller.

For Alexa's immediate check, Hue `on` temporarily follows an accepted pending or
running target, for at most 30 seconds from acceptance. Completion, failure or
expiry restores measurements. Reachability is always measured. Identical active
requests coalesce and do not extend this window. Alexa controls its app's polling
and cache; the bridge cannot push instant app-state changes.

If Alexa keeps an old app state despite receiving the measured value, an
experimental **Alexa compatibility profile** checkbox is available per station
under Set up base stations. Test one station first. It presents a Hue LWB010
light with binary 0/100% behavior; the hardware still only supports ON/SLEEP.
After saving a changed profile, remove only that device in Alexa and rediscover
it so capabilities reload. Its group/routine assignments may need restoring.
This is a compatibility experiment, not a guarantee of app synchronization.

## Wi-Fi, passwords and local API

- **Open Wi-Fi setup** pauses normal control until save or a three-minute timeout,
  then reboots. BOOT held for five seconds is an alternative. Initial setup has
  no timeout. USB information remains available during setup.
- Save a custom **Setup Wi-Fi name / local hostname** under Wi-Fi & local API.
  Use 1–32 lowercase letters/digits, with internal hyphens. `vr-room` gives setup
  SSID `vr-room` and `http://vr-room.local/`. Credentials and key stay unchanged.
  The IP address remains usable if your network does not resolve mDNS.
- Web sessions are HttpOnly/SameSite=Strict, expire after 12 hours and are lost on
  reboot. Passwords use salted PBKDF2-SHA256 (10000 iterations). Password changes
  invalidate other sessions; five failed logins cause a 30-second pause.
- Integrations use **X-API-Key**. It cannot administer passwords, device naming or
  firmware uploads. See the [complete API reference](docs/API.md).
- Forgotten web password: send `LH_RESET_WEB_PASSWORD` plus newline over physical
  USB. Only web authentication is cleared; Wi-Fi, stations and API key remain.

HTTP/NVS are unencrypted; use a trusted local network without port forwarding.
Hue discovery/control deliberately requires no login. USB keys remain in page
memory, never in storage/URLs or sent to GitHub. Improv Serial is not implemented.

## Build and validation

```sh
pio run
g++ -std=c++11 -Wall -Wextra -Werror -I include test/test_core.cpp -o /tmp/lh-core
/tmp/lh-core
python tools/package_release.py
python tools/build_matter.py
espup install --targets esp32 --toolchain-version 1.97.0.0 --export-file .tools/rs-export.sh
. .tools/rs-export.sh
python -m pip install -r tools/requirements-matter-build.txt
python tools/build_rs_matter.py
```

CI also runs native Hue, configuration-migration and OTA-header tests (ArduinoJson
headers from the PlatformIO dependencies). Use `PLATFORMIO_CORE_DIR` or packaging
`--pio-core-dir` for custom installations; `CHROMIUM_PATH` selects the browser.

Manual settings-preserving flash with the release ZIP and esptool 4.5.1:

```sh
python -m esptool --chip esp32 --port COM5 --baud 115200 write_flash \
  0x1000 lighthouse-bridge-bootloader.bin \
  0x8000 lighthouse-bridge-partitions.bin \
  0xe000 lighthouse-bridge-boot-app0.bin \
  0x10000 lighthouse-bridge-app.bin
```

Replace COM5 with your port. Partition locations are unchanged. The UI is embedded;
no filesystem upload is needed. See [WebFlasher details](docs/WEBFLASHER.md),
[dependencies](docs/SOURCES.md), [validation evidence](docs/VALIDATION.md) and
[project instructions](AGENTS.md).
