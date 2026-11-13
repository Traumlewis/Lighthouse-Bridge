# Experimental Matter prototype — 0.3.5-matter

This next test exposes **up to 16 configured Lighthouse V2 stations as separate
On/Off Lights** on the original ESP-WROOM-32 / ESP-32S with at least 4 MB flash.
The owner confirms that the prior single-station 0.1.1 test switches ON/OFF and
shows the correct light type/status. Multiple stations, long-run reliability,
specifically external state changes and this full integration still need
hardware testing. No ESP32 or Echo is attached locally.

The owner reports that full 0.3.0 loaded the web page but returned a memory error
for status, showed no Matter section and no discovery results/logs. Version 0.3.1
replaces the large status JSON allocation/text copy with a bounded snapshot and
streamed response. The Matter section is also independent of general status.
This is a software correction pending owner hardware retesting, not a claim of
confirmed full operation. Discovery now logs queue/start/end; memory diagnostics
report free/largest/minimum heap without credentials or device identifiers.

The owner's 0.3.1 API response confirmed `MATTER_BRIDGE_FAILED`, `ready:false`
and no setup code. Version 0.3.2 corrects startup order: the root stack starts
before the CHIP task creates/resumes saved endpoints and enables them. IDs and
pairing credentials remain intact; no factory reset is required by this change.
Startup errors now remain visible before the code control is rendered. This
correction still needs a real ESP32 retest.

The owner's 0.3.2 device reached Matter/BLE initialization but could not allocate
the BLE worker; its startup then waited forever before starting HTTP. Version
0.3.3 reserves that same 8-KiB task earlier and gates its radio work. HTTP starts
earlier and remains a recovery path when worker/BLE startup fails. Radio controls
are disabled in that state; firmware upload remains available. The owner's 0.3.3
log confirms worker release and ready startup, but the page stays blank. For an
already stalled 0.3.2 or a blank 0.3.3 dashboard, install 0.3.4
using the USB WebFlasher with **Erase device unchecked**; its OTA endpoint cannot
be reached. Existing settings and pairing data remain unchanged.

Version 0.3.4 bounds compressed dashboard socket writes to 512 bytes, checks
partial writes and logs the requested/sent byte counts plus heap diagnostics.
The former single write was about 17 KiB with only about 9 KiB as the largest
reported free block. TCP send pressure is a hypothesis, not a proven cause of
the owner's blank page. A complete send log proves local socket acceptance,
not browser receipt/rendering. STA-derived legacy naming now reads the MAC via
ESP-IDF independently of Wi-Fi netif startup to avoid a zero suffix; custom names
remain unchanged. `/api/matter` (not `/api/metter`) requires a personal-password
web session. No actual hardware fix is claimed without another owner test.

The owner's 0.3.4 test reaches ready and sometimes renders the page, but status
repeatedly cannot allocate its fixed 4512-byte snapshot. One page transmission
failed and a later one completed. Version 0.3.5 captures only existing station and
discovery entries, each in a checked block no larger than 256 bytes, preserving
all 16-entry capacity and coherent measurements. Snapshot metadata stays on stack;
partial allocations are freed on failure and no partial response is returned.
New data-heap status fields/serial diagnostics exclude word-only internal IRAM.
The former largest-block reading cannot prove ordinary allocation availability.
Browser timeouts now display a translated network error instead of numeric 20.
These are software corrections; successful hardware operation needs retesting.

The test bridge name is **Lighthouse-Bridge Matter Test**. The owner wants the
finished bridge named **Lighthouse-Bridge** after testing. Station names are read
from the saved configuration and are not translated or renamed.

## Install and test

1. Existing Wi-Fi, station assignments, API key and web password are retained.
   For a fresh board, the non-blocking protected Wi-Fi setup portal is available.
2. Open [the Matter test WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/matter.html),
   connect the classic ESP32 by USB, and install. **Leave Erase device unchecked.**
3. Close the installer. Read the USB key/address and open the local dashboard.
   First login uses `admin` and the API key, then requires your own web password.
   Later logins use that password. In **Matter setup**, click **Show setup code**.
   If the pairing window is closed, click **Open pairing window (15 minutes)**.
   No USB connection is needed to retrieve the code from the signed-in dashboard.
4. Existing Matter credentials and the first prototype station's identity are
   retained. Check Alexa for the additional lights and the updated bridge name.
   If Alexa caches the old list/name, remove only **this experimental Matter
   bridge while the ESP32 is connected**, then pair it again with the displayed
   USB code. Removing the bridge can remove all its children and their group/
   routine assignments; restore these if necessary. Do not erase the ESP32.
5. For a new pairing, add a Matter device in Alexa and enter the code manually.
   Your Echo must support Matter and share the home network. Test attestation is
   not commercial certification; Alexa may ask to accept an uncertified device.
6. Test every station individually: ON, then SLEEP, and its Alexa status. Check
   that no other station switches. If desired, put the lamps into an Alexa group
   and test collective ON/SLEEP. The bridge does not create a virtual group light.
   Allow for sequential BLE work and booting/confirmation; a sent command is not
   the same as an observed steady ON (raw 11) or SLEEP (raw 0).
7. Report the station count and any switching/status issue. Useful serial lines
   start with `Matter command`, `Matter measurement`, or `Station`. Do not share
   setup codes, API keys or credential-bearing logs.

The dashboard now includes discovery, the station editor, real measurement status,
the local HTTP API and Wi-Fi firmware upload using the same production services.
Saving station settings preserves Matter identities and restarts after 1.5 seconds.
No power command is sent during startup/restart. Matter-only profile controls
replace Hue-specific help; saved Hue compatibility settings remain intact.
Download the separate **Matter .lhota** from the Matter page for later dashboard
updates. A transition from 0.2.0 still requires USB because that version had no
upload service. Return to stable using its USB WebFlasher without erasing.
Existing Hue entries are unavailable while the Matter firmware runs.

The setup-code endpoint requires a personal-password web session. Neither API
keys nor bootstrap sessions can access it; general status never includes it.
Opening a pairing window requires the same session and JSON/Origin checks. It
runs on the CHIP task, advertises over DNS-SD only and never erases fabrics or
writes station power. An already-open window is not extended by duplicates.
The code stays in page memory, is cleared on logout and is not stored in URLs or
browser storage. Local HTTP remains unencrypted and is for the trusted LAN only.

## Storage and identities

Both **1.875 MiB app slots**, NVS `0x9000..0xdfff`, OTA metadata and the unused
filesystem region retain the exact stable partition table. USB writes only four
separate files at `0x1000`, `0x8000`, `0xe000`, `0x10000`. No merged-at-zero image
is offered. A separate branded `.lhota` contains only the validated app image. The application must fit `0x1e0000`.

The shared station parser loads the stable record; explicit editor/API saves update it. The
separate `lhmatter` namespace keeps per-board commissioning credentials and an
atomic `endpoints` registry binding full stable Hue identity + MAC to a Matter
endpoint. The original first station retains endpoint 1; aggregator 2 is reserved.
Other endpoints start at 3 and increase monotonically. Reordering never renumbers
survivors; a replacement MAC or new identity gets a new endpoint. Retired endpoints
are not reused. Corrupt/exhausted registries fail closed, without erasing settings.
Current+candidate checkpoints can hold 32 bindings while active capacity remains
16. Stable local IDs remain 1–16 and are distinct from Matter endpoint IDs.

The original single-station migration assumes its saved assignment was retained.
If you replaced assignments while running stable firmware between tests, refresh
the experimental Alexa pairing rather than assuming its cached device is unchanged.
Matter also persists fabric data; long-run NVS behavior and returning between
framework versions remain hardware checks.

## USB and builds

`LH_MATTER_INFO\n` at 115200 baud returns a `LH_MATTER_INFO` JSON frame with protocol
1, version, bridge name, readiness/error, Wi-Fi, pairing state/code and a measured
`stations` list. Legacy first-station fields remain for older readers. It exposes
no station MAC, stable API key or Wi-Fi password. Codes stay in browser memory,
never in URLs, storage or network calls. Disconnect USB reading before flashing.
Language changes retain the USB connection and experimental consent checkbox.

Install Arduino CLI **1.6.0**, then:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install 'NimBLE-Arduino@2.5.2' 'ArduinoJson@6.21.5' 'WiFiManager@2.0.17'
python3 tools/build_matter.py
```

The stable PlatformIO environment remains Arduino ESP32 2.0.17. The isolated build
uses 3.3.12 with CHIPoBLE commissioning disabled, leaving NimBLE 2.5.2 to the one
sequential Lighthouse worker. The sketch includes production `src/main.cpp` with the Matter adapter replacing
Hue. There is no second worker, authentication or OTA implementation. The build
gzip-compresses the shared UI, uses C++-only LTO and compiles out WiFiManager
debug output. C must remain non-LTO with the pinned SDK.
The root request/measurement logic remains shared. Boot and reconnect never send
automatic power commands. Matter command interception queues work; only observed
OnOff/Reachable updates are reported on the Matter task.

The build validates classic ESP32 images, both slots, all partition entries,
sector bounds and per-image hashes. `dist/matter/` contains split images, manifest,
checksums, source/notes/licenses ZIP and build metadata. Build both packages before
`tools/build_pages.py`; the existing Pages workflow publishes separate stable and
experimental choices. Software tests do not establish successful multi-station
operation, runtime RAM margins, or Alexa discovery/cache behavior.
