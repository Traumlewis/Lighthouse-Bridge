# Lighthouse Bridge — Tasmota Matter prototype 0.1.5-tasmota

This separate experimental firmware combines Tasmota's flash-solidified Berry
Matter implementation with the Lighthouse Bridge dashboard, authentication,
local API, sequential NimBLE worker and validated firmware updater. It targets
the original dual-core ESP-WROOM-32 / ESP-32S with 4 MB flash. No ESP32/Echo is
attached in development; software checks are not a successful device test.

Owner 0.1.0 evidence confirms HTTP recovery and roughly 72–74 KiB free byte heap
at failed startup. Matter fails with import_error: the minimized binary omitted
Berry's native mDNS module. Version 0.1.1 restores only that binding, keeping generic
Tasmota discovery disabled, and validates linked native modules before packaging.
Discovery/code controls stay unavailable while Matter startup has failed. Successful
Alexa pairing and long-run operating memory remain unverified.
A working 0.1.0 dashboard can receive the new Tasmota .lhota through its authenticated
firmware-update controls; USB with erasing unchecked remains an alternative.

Version 0.1.2 adds `Matter pairing diag:` serial summaries every 15 seconds.
They contain boot-lifetime packet/error counters, last secure/IM opcodes and
commissioning command masks, window/announcement flags, fabric count without IDs,
local IPv6 presence and free byte heap. No peer
addresses, codes, packet contents or raw exceptions are logged. Opcode 0xffff
means unseen; masks set bit N when command N is received in cluster 0x30 or 0x3e.
A completion counter records local handling, not Alexa receipt. The owner now
confirms 0.1.1 startup, four-device discovery and one confirmed ON command, but
Alexa returns GS014 during setup. This diagnostic release does not claim to fix it.
For a retest, capture serial output before opening the commissioning window,
through the Alexa error, and for another 20 seconds. Share the `Matter pairing diag:`
lines; other serial output may contain setup credentials and must stay private.

Version 0.1.3 reapplies IPv6 after WiFiManager's interface restart, avoids duplicate
Matter startup, and updates delegated A/AAAA addresses when local addresses change
or Wi-Fi reconnects. Existing services, identities and fabrics remain intact. The
owner's 0.1.2 log reaches locally handled CommissioningComplete but Alexa still
returns GS014, with IPv6 absent and an earlier mDNS error. These lifecycle changes
address verified code defects; a GS014 fix still needs hardware confirmation.
Retest with erasing disabled, then capture the diagnostic lines during setup as
above. Do not reset all settings or replace saved pairing identities for this test.

Version 0.1.4 offers **New code & open pairing window (15 minutes)** in Matter
setup. Click it before adding the bridge in Alexa, then show/copy the returned code.
Each manual activation replaces the previous window and code. Pending duplicate
requests coalesce. Codes expire when the window closes or the bridge restarts;
they are not written to storage. Existing pairings, bootstrap credentials and
station identities stay intact. The local-owner action uses the engine's Basic
window/verifier path; it is not a controller-created Enhanced Commissioning Window.
A closed window intentionally has no displayed code. Merely showing a code does
not open a window. The owner 0.1.3 log shows IPv6/mDNS recovery but no open window
or incoming packets during the new Alexa search. Actual new-code pairing still
requires a device test. No reset is needed to try this workflow.

Version 0.1.5 adds bounded `Matter IM diag:`, `Matter IM trace:` and
`Matter IM errors:` serial metadata. It reports attribute/read/write/command paths and status
codes without their contents, plus subscriptions and controller status failures.
The newest eight pending paths are kept; discarded history is counted and the
first/last errors are retained. R/W/C:endpoint/cluster/id=status uses numeric IDs:
000=handler success, 100=ignored/deferred, 101=attempt without result; 201..2ff
contains a commissioning response error enum after subtracting 0x200. Optional
unsupported attributes may be normal. A success does not prove Alexa accepted it.
For a retest, use one new-code pairing attempt and record the `Matter pairing diag:`
and all `Matter IM` lines through 20 seconds after Alexa's result. Other serial
output can contain setup credentials. Owner 0.1.4 reached local completion but
still received GS014; this release adds evidence, not a verified pairing fix.

## Included behavior

- Configure 0–16 Lighthouse V2 stations with fixed local slots, stable Matter
  endpoints and their existing names. Discovery and manual refresh are read-only.
- Each assigned station is an On/Off Light below the Matter bridge. Alexa ON/OFF
  queues the existing bounded BLE job. OFF sends SLEEP (`0x00`); ON sends `0x01`.
  STANDBY is never sent. Nothing sends power on boot, reconnect or measurement.
- OnOff and Reachable come from measured station state. Command acceptance does
  not change a measurement. Toggle requires a fresh measurement. Command retries,
  coalescing, confirmation checks and assignment-generation protection are shared
  with production firmware.
- The full English/German dashboard includes assignments, discovery, measured
  status, local API key, password setup/recovery, hostname, Matter setup code,
  a 15-minute commissioning window and local firmware uploads.
- First login uses admin and the device API key, then requires a personal web
  password. Later logins use that personal password. Integrations use X-API-Key;
  private account/Matter/OTA routes still require the personal web session.
- Physical USB uses the existing LH_INFO and LH_MATTER_INFO protocols through
  one serial owner. No generic Tasmota console commands are enabled.

## Install and return

Use the separate [Tasmota WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/tasmota.html).
Keep **Erase device unchecked**. The installer uses exactly four images at
0x1000, 0x8000, 0xe000 and 0x10000. The complete partition table is unchanged:
both app slots remain 1.875 MiB, NVS stays at 0x9000..0xdfff, and the existing
192-KiB data partition stays at 0x3d0000. No filesystem image or merged BIN is
used. Saved WLAN, API key, personal password and station assignments remain in NVS.

After startup open the reported IP address, sign in and use **Matter setup**.
Pair the test bridge **Lighthouse-Bridge Matter Test** on the same home network.
The Tasmota engine has separate fabric storage; it cannot resume an Espressif
Matter fabric. Pair the prototype anew, removing only the previous experimental
bridge from Alexa if necessary. That can require restoring its groups/routines.
Existing SDK fabric data is retained on the ESP32 for return, but removing a
controller's device also removes its side of that pairing. Do not erase NVS.

The shared endpoint registry and setup passcode/discriminator are retained;
aggregator endpoint 2 and the former first-light endpoint 1 are preserved. Other
endpoint IDs remain monotonic and bind by stable identity plus MAC, not slot.
Names are retained in configuration; the Matter label uses the same UTF-8-safe
32-byte prefix as the SDK prototype.

Tasmota settings/fabrics are isolated in the existing filesystem. It is mounted
without formatting. Only a completely erased data partition may be initialized;
nonempty invalid storage fails closed and requires diagnosis, not auto-erasure.
No automatic partition expansion, raw Tasmota OTA, unauthenticated webserver,
MQTT management, external startup scripts or Tasmota reset/erase are enabled.
Lighthouse owns Wi-Fi and the protected setup portal.

Future updates use the separate `lighthouse-bridge-0.1.5-tasmota.lhota` through
the authenticated dashboard. The receiver validates product, chip, length and
SHA-256 before selecting the inactive slot. Return to stable or the SDK experiment
through the respective four-part USB installer with erasing unchecked.

## Build and source

The prototype distribution is GPL-3.0-or-later because it links Tasmota. Its ZIP
includes the full modified Tasmota source and the Lighthouse source/build patches;
original notices/licenses are retained. Stable Lighthouse source retains its
existing license. See LICENSE.txt and the ZIP's source/LICENSE.

Pinned inputs:

- Tasmota: `6aadc3f171f5b501cc325dcd78daf3d625b01d32`.
- Tasmota ESP32 platform: release `2026.11.50` (Arduino 3.3.12 fork).
- Python 3.12, pioarduino 6.2.0.post1, Zopfli 0.2.3.post1.
- NimBLE-Arduino 2.5.2, ArduinoJson 6.21.5, WiFiManager 2.0.17.
  The NimBLE config header enforces the sequential client/scanner policy. One
  internal ble_gap.c declaration is aligned with its existing uint8_t definition
  for GCC LTO compatibility; function behavior, retries, stacks and pools remain.


From the repository, first build/package stable firmware so its libraries and
reference partition binary exist. Then:

```sh
python3.12 -m venv .tools/tasmota-venv
.tools/tasmota-venv/bin/pip install -r tools/requirements-tasmota-build.txt
.tools/tasmota-venv/bin/python tools/build_tasmota.py
```

To rebuild the complete source ZIP, enter `source/`, install those Python
requirements, and run `python tools/build_tasmota.py --prepared --source-tree
tasmota-upstream --pio /path/to/pio`. The script rewrites absolute include paths
for the extracted source directory. The prepared tree contains the exact upstream
source and patches; network access is still needed for pinned toolchain/framework
packages on their first installation. A packaged reference partition binary is
provided for layout validation when rebuilding without a stable build.

Outputs are in dist/tasmota: split images, .lhota, manifest, checksums, build
metadata and the complete-source ZIP. No oversized image may be packaged.
Static RAM/link size does not prove free byte heap during BLE/Matter operation.

## Verification still required on hardware

Check startup/recovery, initial WLAN setup, all four physical stations, actual
Alexa pairing/commands, external-state reporting, persisted fabrics after reboot,
password/API separation, OTA and return across frameworks. Sixteen configured
endpoints in host tests do not prove sixteen simultaneous physical stations or
long-run memory/RF reliability. Retest with modem sleep enabled and report
sanitized Memory ready / Matter diagnostics, never access keys or device MACs.
