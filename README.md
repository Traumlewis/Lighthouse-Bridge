# Lighthouse Bridge — ESP32

Control **four Valve Lighthouse V2** base stations with an **ESP-WROOM-32 / ESP-32S
CH340 USB-C board** and an **Echo Dot (5th generation)**. **On = ON (`0x01`),
off = SLEEP (`0x00`), never STANDBY.** No extra phone companion app is required.

**[WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/)** ·
[German UI](https://traumlewis.github.io/Lighthouse-Bridge/?lang=de) ·
[Builds](https://github.com/Traumlewis/Lighthouse-Bridge/actions) ·
[Validation report](docs/VALIDATION.md)

Repository documentation is maintained in **English**. Both the public installer
and the device dashboard default to English and offer German as a second language.

## What's new in 0.2.2

BLE error **574 / 0x023e** means **connection establishment failed** (HCI 0x3e).
It does not identify a specific interference, range or hardware cause. On this
error the next attempt first performs a two-second passive scan for the assigned
MAC and uses its freshly advertised address type for that job. Retry timing varies
to avoid repeatedly hitting the same radio timing. Saved assignments stay intact.
Serial diagnostics identify the failed phase, attempt and code, plus the recovery
scan's RSSI/address type or missing advertisement. No power writes occur in a read
or recovery scan. Commands still have at most three attempts; reads have two.

The 0.2.1 measurement rules remain: one failed cycle retains a usable previous
measurement labeled **last reading**, with a five-second retry. Two failed cycles,
no previous reading, or age of at least 60 seconds means unreachable. Failed reads
never refresh measurement timestamps. Real-board reliability still needs a retest.

## Features introduced in 0.2.0

- Separate flash regions preserve NVS, including Wi-Fi, assignments and passwords,
  when **Erase device is not selected**.
- BLE states are read at startup, after commands and periodically. A manual refresh
  queues reads without power commands. Requested targets and measurements remain
  separate; Alexa receives the same measurements in its Hue queries.
- English/German device dashboard with a saved language preference.
- First login with the API key, followed by a required personal web password.
  The API key stays separate and visible under **Wi-Fi & local API** after login.
- USB display of the key and Wi-Fi address: close the installation dialog after
  flashing, or select **Read key & address**.

**Important update correction:** The old merged image contains `0xff` padding over
NVS. Writing it at `0x0` overwrites settings even without a full erase. Since 0.2.0,
the WebFlasher uses four separate parts instead. The downloadable **merged BIN at
`0x0` is for factory installations only**. The partition layout has not changed.

The owner confirmed successful startup and basic operation of 0.1.1. The later
features have passed builds and simulated-backend/USB tests, but complete testing
on the stations and Echo is still pending. See the validation report for evidence
and the reported intermittent connection failures.

## Hardware and first setup

Use a classic dual-core ESP32 with at least **4 MB flash** and 2.4 GHz Wi-Fi. This
firmware is not for ESP32-S2/S3/C3. A USB data cable is sufficient; no extra wiring
is required. Keep the base stations on their normal power supplies and place the
ESP32 within BLE range of all four.

1. Open the WebFlasher in Chrome or Edge **on a computer**, connect the board,
   confirm the supported hardware and choose **Connect & install**.
2. For an update, **leave Erase device unchecked**. Erasing is only for an
   intentional fresh start and deletes all saved settings.
3. Close the installation dialog. The USB information panel reuses the granted
   port and displays the individual key and setup Wi-Fi name. Alternatively,
   choose **Read key & address**. Close any other application using the serial port.
4. On first setup, connect to `Lighthouse-Setup-XXXXXX` using the key as its password.
   Open `http://192.168.4.1`, select your home Wi-Fi and save. Keep USB connected;
   the installer displays the local web address once Wi-Fi connects.
5. On your home network, open that address. The username is **`admin`**; for the
   first login use **the API key as the password**. Then choose your own web
   password: 8–128 bytes, different from the API key.
6. Discover the four `LHB-…` devices, assign each to exactly one slot and save.
   Discovery supplies the correct BLE address type automatically.
7. States are read automatically. Test ON/SLEEP on one station first, then the
   group. Startup, reboot and reconnection never send power commands automatically.

If no USB port appears, check the CH340 driver and data cable. If connection stalls,
hold BOOT, briefly press EN/RESET, and release BOOT once the flasher connects. For a
normal restart press only EN/RESET. The key is also printed at 115200 baud in the
serial console. USB device information requires firmware 0.2.0 or newer; update
older firmware first.

## Measured state and Alexa

The ESP32 reads each configured station's power characteristic:

| Raw value | Dashboard | Hue `on` |
| --- | --- | --- |
| `0x00` | SLEEP | false |
| `0x01`, `0x08`, `0x09` | Starting | true |
| `0x0b` | ON | true |
| `0x02` | STANDBY (read only) | false |
| No successful measurement | Unknown | last known value, `reachable: false` |

A single BLE worker processes one connection at a time. Commands have priority and
up to three attempts. Reads run at startup, after commands and at a target interval
of 15 seconds; starting stations are checked after about two seconds, unreachable
ones after about 30 seconds. A single failed read cycle triggers a five-second
retry. Other work and connection attempts can extend these intervals.

`requested` is a target, **not a measurement**. The dashboard and Hue responses use
`observed`. A single failed read cycle preserves a recent previous value with a
warning and its original timestamp. Repeated failed cycles or age of 60 seconds
make it unavailable. The dashboard polls the ESP32's stored data every 2.5 seconds.

Keep the Echo Dot and ESP32 on the same home network without client isolation.
Ask Alexa to discover devices. It finds four lights and the group light
**`VR Basisstationen`**. This established group name is retained for compatibility;
existing device names are also preserved. New slots default to “Base station 1”,
etc. The virtual group is on when all assigned stations are active, and reachable
when all have usable measurements.

**Alexa receives the same measurements when it queries the local Hue bridge.
This protocol cannot push state changes into the Alexa app.** The app may cache
its switch until Alexa queries again. Immediate display synchronization cannot
be guaranteed. Alexa also acknowledges commands before BLE completes; check the
dashboard for failures. Hue emulation needs testing with the installed Echo firmware.

## Wi-Fi, web password and API key

- Change Wi-Fi under **Wi-Fi & local API → Open Wi-Fi setup**, or hold BOOT for five
  seconds after startup. Wait for active BLE work to finish. The manual portal
  pauses normal control and reboots after saving or after a three-minute timeout.
- If saved Wi-Fi is unavailable at startup, the setup portal has no timeout. USB
  information remains available. Wi-Fi is stored in ESP32 NVS, which the separate
  update parts preserve.
- The web login uses an HttpOnly/SameSite session. The personal password is stored
  only as a PBKDF2-SHA256 hash with an individual salt and 10000 iterations. It is
  never displayed or sent over USB. Changing it invalidates other sessions.
  Browser sessions last at most 12 hours and are lost on reboot.
- The **API key** remains the `X-API-Key` integration credential and setup Wi-Fi
  password. After a personal web password is set, the key no longer works for web
  login. Signed-in users can still see it under **Wi-Fi & local API**.
- Forgotten password: send `LH_RESET_WEB_PASSWORD` followed by a newline over USB
  at 115200 baud. Only the web password and sessions are cleared. Sign in with the
  API key and choose a new password. Wi-Fi, the key and assignments stay intact.

HTTP and NVS are unencrypted. Hue endpoints intentionally allow local Alexa access
without login. Use a trusted home network without internet port forwarding. USB
keys and addresses stay in page memory; they are not sent to GitHub or saved in
browser storage.

## Local API and future Pebble support

See the [API reference](docs/API.md). A future Pebble plugin can use these endpoints
with LocalNetwork permission and `X-API-Key`. The plugin, OTA updates and Improv
Serial are not implemented yet.

## Build, test and flash manually

Open this project in PlatformIO and use the `esp32dev` profile:

```sh
pio run
pio run -t upload --upload-port COM5
pio device monitor --port COM5 --baud 115200
```

Replace `COM5` with your port. For a manual **settings-preserving** update using
the ZIP contents and esptool 4.5.1:

```sh
python -m esptool --chip esp32 --port COM5 --baud 115200 write_flash \
  0x1000 lighthouse-bridge-bootloader.bin \
  0x8000 lighthouse-bridge-partitions.bin \
  0xe000 lighthouse-bridge-boot-app0.bin \
  0x10000 lighthouse-bridge-app.bin
```

Writing `lighthouse-bridge-esp32-merged.bin` at `0x0` overwrites NVS; use it only
for a factory installation. Two application slots are reserved for future work;
OTA is not implemented. The dashboard is embedded in firmware and needs no
filesystem upload.

```sh
g++ -std=c++11 -Wall -Wextra -Werror -I include test/test_core.cpp -o /tmp/lh-test
/tmp/lh-test
python tools/package_release.py
python tools/build_pages.py
python -m unittest discover -s test -p 'test_pages.py' -v
node test/serial_info.cjs
npm install
npm test
```

Use `PLATFORMIO_CORE_DIR` or packaging `--pio-core-dir` for a custom installation,
and `CHROMIUM_PATH` for the browser executable. Browser/USB simulations do not
replace hardware tests. See [WebFlasher details](docs/WEBFLASHER.md),
[sources and dependencies](docs/SOURCES.md), and [project instructions](AGENTS.md).
