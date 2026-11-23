# Lighthouse Bridge local API

The active firmware is rs-matter. See the [setup guide](WEBFLASHER.md) for
installation and board selection.

Starting with LH-Bridge 0.1.29, the `version` field is a numeric version
string such as `"0.1.29"`. Earlier rs-matter versions included the
`-rs-matter` suffix.

Base address: `http://<ESP32-IP>`. Integrations use `X-API-Key: <key>` as before.
The web dashboard uses an HttpOnly session instead. HTTP Basic has been replaced
by the web login. Mutations require `Content-Type: application/json`. CORS is not enabled.

## rs-matter 0.1.23 network names and station information

With no valid custom name the setup SSID is `Lighthouse-Bridge-Setup-XXXXXX`
and the network hostname is `lighthouse-bridge-xxxxxx`.

Personal-session `POST /api/station-info` with the saved station `id` and
`mac` returns 202 with a numeric `job`. Poll personal-session
`GET /api/station-info` for the same `job` and `pending: false`. The
successful `info` object contains `type` and any available attributes:
`name`, `firmware`, `channel`, `manufacturer`, `model`, `serial` and
`hardware`. Since 0.1.26, `serial` may be a string for printable serials;
short binary serials remain numeric. Unavailable fields are omitted. The transient read job shares
the same BLE queue with Identify and uses the existing Origin and JSON
checks. API-key and bootstrap sessions cannot access these routes.

## rs-matter 0.1.20 Identify and Remote Mode

Personal-session `POST /api/identify` accepts a saved station `id` and matching
`mac` and returns HTTP 202 with a numeric `job`. Poll the personal-session
`GET /api/identify` and check for that `job`, `pending: false`, and an empty
`error` on success. V2 devices without the Identify characteristic return
`IDENTIFY_UNSUPPORTED` as the job result. Since 0.1.22, Identify may also
target a sleeping station. The dedicated LED command can wake the station.
The bridge sends no separate power or channel command.

`GET /api/buttons` also includes `wake_pins`. Personal-session
`POST /api/remote` with `{"confirm":true}` starts the BLE-only sleep mode,
after validating both saved bindings and that a station is assigned.
Wi-Fi and dashboard access resume after holding both buttons for five seconds.
Both POST routes use the existing Origin and JSON checks.

## rs-matter 0.1.19 room and maintenance

Personal-session `GET /api/account` includes `room_name` (empty means the localized
"Your VR room" default). No room name or API key is returned to bootstrap sessions.

`POST /api/room` accepts exactly `{"name":"Leas VR room"}`: nonempty printable
UTF-8 up to 64 bytes, persisted independently of the hostname. Returns
`{"saved":true,"name":"Leas VR room"}`. Invalid input returns `ROOM_NAME_INVALID`;
storage failure returns `SAVE_FAILED`. No reboot or station command occurs.

`POST /api/maintenance` requires a personal-password session and the matching
Origin when supplied, plus JSON. It accepts `{"action":"restart"}` for a
normal restart. Since 0.1.24, `{"action":"factory_preview"}` returns
200 with a `reset_key` valid for five minutes in the same session and
leaves the active key and NVS untouched. To reset, submit
`{"action":"factory_reset","confirm":true,"reset_key":"<preview key>"}`.
Missing confirmation returns `RESET_CONFIRM_REQUIRED`. A missing, replaced
or expired preview requires a new preview (`RESET_PREVIEW_REQUIRED`).
Busy scans/commands, setup, OTA or an existing restart can return 409
`BLE_BUSY`. A failed durable reset record write returns 500 `SAVE_FAILED`.
Accepted restarts/resets return 202 `{"accepted":true,"restarting":true}`
and reboot after at least 1.5 seconds and physical BLE idle.

Restart preserves settings. Factory reset erases the standard NVS partition at the
next boot, before bridge/Matter/BLE startup: home Wi-Fi, API key, web login, room,
station/endpoint assignments, GPIO bindings and Matter fabric data. Other storage
partitions, firmware slots, partition table and OTA selection remain untouched.
The confirmed preview key becomes the new API/setup key. It remains recoverable
over physical USB. Repeat initial setup and Matter pairing. No power command is sent automatically. Storage failure
at boot stops initialization with `FACTORY_RESET_STORAGE_FAILED`; power-loss recovery
and actual device reset remain hardware checks. Do not invoke these routes merely
to inspect settings.

## rs-matter 0.1.16 setup reboot and physical diagnostics

`POST /api/wifi` and BOOT setup requests consume a one-shot setup flag
on a fresh boot before Matter/BLE initialization. Existing authentication/Origin
checks apply. Request persistence failure returns `500 SAVE_FAILED` without
pausing the running bridge. The saved home/local choice is unchanged; timeout or
close reboots into it. The browser preserves the key for the setup connection.

Physical USB command `LH_WIFI_INFO\n` returns `LH_WIFI_INFO {protocol:1,...}`
with AP mode/config error, auth/cipher/channel/PMF, `key_matches`, `name_matches`,
client/join/leave counts, last disconnect reason and byte-heap diagnostics. Match
fields compare driver configuration to the stored expected values. No raw keys,
SSIDs, device MACs or protocol credentials are returned. This command is read-only;
existing USB readers do not automatically add it to their queries.

## rs-matter 0.1.15 Local Wi-Fi Mode

Local Mode runs the same authenticated dashboard/API at
`http://192.168.4.1/` on the bridge's own protected Wi-Fi. Station control,
discovery, config saves and personal-session OTA remain available without a
router. Matter is paused: `matter_ready:false`, `matter_paired:false` and
`matter_error:"LOCAL_MODE"` describe the inactive runtime, not erased fabrics.
The actual pairings are retained for resuming home mode.

Status/account/private Matter/USB information adds `network_mode:"home"|"local"`.
Status IP and the USB URL use the AP address in local mode; `wifi_connected`
continues to indicate an actual home/STA connection and remains false on AP-only.
General status never contains API keys, Wi-Fi passwords or Matter setup codes.

`POST /api/network`, body `{"mode":"home"}`, requires a personal web session and
Origin/JSON checks, never API key/bootstrap authorization. It atomically selects
home mode and returns `{"saved":true,"restarting":true}` if a restart is needed.
An already-home request returns `restarting:false`; invalid mode returns
`400 NETWORK_MODE_INVALID`. Pending power/scan/OTA work returns `409 BLE_BUSY`.
Saved router credentials are reused, or the setup portal opens if unavailable.
Local mode is selected through WiFi Setup -> Local Mode and its confirmation POST;
GET requests only display the form. The mode persists until explicitly changed.

AP-local key regeneration and device naming additionally return `restarting:true`
to apply the new Wi-Fi credentials. Copy the new key before reconnecting; it is
not put in a URL or storage. The web password and Matter pairings stay unchanged.

## rs-matter 0.1.18 GPIO buttons

Personal-password-session `GET /api/buttons` returns `pins` (the compiled board's
supported GPIO list) and exactly two `buttons` records, each with integer `pin`
and action `disabled`, `all_on` or `all_off`. Disabled uses pin -1. POST on the
same route accepts `{ "buttons": [{"pin":25,"action":"all_on"},
{"pin":26,"action":"all_off"}] }` (example pins for classic ESP32 only).

Both methods reject API-key/bootstrap authentication. POST requires same-Origin
JSON, supported distinct GPIOs and a valid complete pair. Invalid data returns
400 `BUTTON_CONFIG_INVALID`; failed atomic persistence returns `SAVE_FAILED`.
Saving changes input configuration only, never power state. Inputs use pull-ups
and momentary contacts to GND, 40-ms debounce, release-to-arm and no hold repeat.
All off uses each station's configured OFF state. Presses during maintenance are
rejected by the existing command gate and never replayed later. Both buttons are
disabled by default; malformed saved configuration disables both.

## Web authentication (not integration endpoints)

- `POST /api/login`, body `{"password":"…"}`. Before a personal password exists,
  the API key is accepted. Later logins accept only the personal password.
  Returns `{"password_required":true|false}` and the `LHSESSION` cookie.
  Five failed attempts trigger a 30-second block (`429`).
- Bootstrap sessions must set a personal password first; normal protected requests
  return `428 PASSWORD_REQUIRED` until then.
- `POST /api/password`, body `{"password":"…"}`: 8–128 bytes, different from the API
  key. Requires a web session, **not** `X-API-Key`. Atomically saves the salt and
  PBKDF2-SHA256 hash in NVS, invalidates old sessions and issues a new one. Returns
  `{"saved":true}`. The password is never returned.
- `GET /api/account`: requires a web session, including bootstrap sessions. Returns
  `password_required`, `setup_ssid`, `hostname` and `device_name` (empty until customized). `api_key` is included only after password
  setup. API-key header authentication does not grant access to this endpoint.
- `POST /api/logout`, body `{}`: removes the web session. The cookie is HttpOnly,
  SameSite=Strict and has a 12-hour lifetime; server sessions also have a 12-hour
  inactivity limit. Reboot discards all sessions.

The dashboard hides detailed errors, background-read phases, last-command history
and measurement ages by default. Measured badges, unavailable/first-read status
and queued/sending power commands remain visible. Debug text scrolls within a
reserved card area; toggling does not change the layout. Triple-click its firmware version within 1.2 s
to toggle debug details. This only changes browser presentation; all API fields
and serial diagnostics remain available and BLE behavior is unchanged.

### rs-matter 0.1.12 account and OFF preferences

These options apply to the personal web login.

- Public `GET /api/login` returns boolean `username_enabled` and, since 0.1.26,
  boolean `password_set`. Since 0.1.35 it also returns the firmware `version`.
  First setup and migrated accounts default to having
  usernames disabled. The endpoint never returns the account name.
- If enabled after personal-password setup, `POST /api/login` additionally needs
  `"username":"…"`. Incorrect names/passwords share the existing failure/throttle
  behavior. Bootstrap login needs no name.
- Personal-session `GET /api/account` additionally returns `username_enabled` and
  `username` (empty when disabled). It still never returns the password/hash/salt.
- Personal-session `POST /api/username`, body
  `{"username_enabled":true,"username":"owner"}`, enables or changes the login
  name. Allowed names have 1–32 ASCII letters, digits, dots, hyphens or underscores.
  Use `{"username_enabled":false}` to disable and clear it. Returns `{"saved":true}`;
  invalid input returns `400 USERNAME_INVALID`. The salt/hash/name/settings are
  persisted as one record before live state changes. Other sessions are invalidated;
  the caller gets a replacement cookie. Password updates preserve these settings.
- Personal-session `POST /api/key`, body `{"confirm":true}`, generates and returns
  a new 24-hex `api_key`. Missing/false confirmation returns `400 KEY_CONFIRM_REQUIRED`.
  Old integration keys immediately stop authenticating. The new key is also the
  password for future setup access points; saved router Wi-Fi and the personal
  web password stay unchanged. No reboot, power or pairing action is triggered.
- Both new POST routes reject API-key and bootstrap authentication and enforce
  existing Origin/JSON checks. Keep returned credentials in memory, never URLs/logs.
- Station configuration/status adds `off_state:"sleep"|"standby"`. Missing NVS
  values default to sleep; an omitted edit field preserves an existing selection.
  Invalid values/types return `400 CONFIG_OFF_STATE`. The normal config-save
  restart still applies, preserving endpoint identities and sending no power.
- `POST /api/power` accepts `state:"off"` to use each selected station's saved
  choice (also for `station:"all"`). Explicit on/sleep/standby remain exact commands.
  Alexa OFF/toggle-off uses each station's saved choice. Measured state remains
  independent; raw 2 confirms STANDBY, 0 confirms SLEEP, 11 confirms ON.
- The rs dashboard hides station IDs/MACs and history until debug is enabled;
  unlike the earlier fixed-height UI, cards expand/shrink with debug visibility.
  All API measurement fields remain available independently of debug.
  Version 0.1.13 additionally hides the manual refresh button outside debug;
  the refresh endpoint and automatic read scheduling stay available.

Experimental 0.3.7 station-save success additionally returns `restarting:true`
and the assigned public station list. The live config/status keeps the previous
assignment until reboot. Wait for restart and refresh before addressing any slot.
The restart grace period starts after response serialization; acceptance does not
prove subsequent Matter startup. Registry checkpoint and station NVS writes keep
the same ordering/keys. Failure leaves the current live assignment intact; an
already stored transition checkpoint can safely advance reserved endpoint IDs.

Experimental 0.3.5 additionally reports `free_data_heap` and `largest_data_block`
for internal byte-addressable memory (`MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`).
Existing `free_heap`, `largest_free_block` and `min_free_heap` retain Arduino's
internal-heap meaning, which can include word-only IRAM on classic ESP32. Do not
use those older figures as a guarantee that a normal data allocation will succeed.

Experimental 0.3.3 status includes `ble_worker_available` and `ble_startup_error`.
When the worker/driver is unavailable, radio mutations (`/api/power`, `/api/scan`,
`/api/refresh`) return 503 with the startup error, without queueing work. Account,
web login and personal-session OTA recovery remain available. Failure to start
the worker does not grant API-key access to private setup/OTA endpoints.

## Matter setup (personal-password session only)

- `GET /api/matter`: returns `bridge_name`, `version`, `ready`, `paired`,
  `pairing_code` (null until ready), `error`, `commissioning_open`,
  `commissioning_pending`, `commissioning_error` and non-secret `startup_phase`.
  On C3, `commissioning_quiet_ms` is included while background work is
  deferred and gives the remaining time. Use `Cache-Control: no-store`;
  do not save or include the returned code in URLs/logs. This route accepts
  neither API-key authentication nor bootstrap sessions.
- `POST /api/matter/commissioning`, JSON `{}`: same personal-session restriction
  and Origin/JSON checks. Returns `202 {"accepted":true}` when scheduled, or
  `409 MATTER_NOT_READY`. Poll GET for completion/error. Opens a Basic
  Commissioning Window for 15 minutes over DNS-SD only; existing open/pending
  windows coalesce. No fabric reset, BLE commissioning or station power writes.
- The personal-session account response identifies Matter with `alexa_protocol`
  and `matter_ready`, independently of status. It never includes the pairing code.
- Matter status is streamed in bounded chunks from one coherent snapshot. It
  includes `largest_free_block` and `min_free_heap` alongside `free_heap` for
  diagnosis. No whole-response JSON allocation is required.
- Matter status adds `alexa_protocol:"matter"`, `matter_ready`, `matter_paired`
  and `matter_error`. General status never includes the setup code;
  `alexa_started` denotes Matter readiness in this build. The API's measured
  station fields retain their production meaning; The legacy field `alexa_target_pending` is false.
- Station configuration saves checkpoint persistent
  Matter identities, save assignments, pause new BLE work and restart 1.5 seconds
  after responding. Refresh integrations and sign in again after the restart.
  Existing station endpoints survive reordering. Replacing a physical station
  receives a fresh endpoint; no automatic power command is sent on restart.
- The same OTA security/container checks apply. Download the board-specific
  `.lhota` from the main WebFlasher.

## GET /api/status

Returns version, IP/hostname, Wi-Fi and Matter readiness, uptime, free heap, a dynamic list of up to 16
entries, discovery state and results. `alexa_started` reports Matter readiness.
Top-level fields include `max_stations:16`, `maintenance`, `setup_ssid`,
`ota_supported` and `ota_product`. Station IDs are fixed local slots 1–16. New entries receive the lowest free ID.
Deletion/reordering never renumbers survivors; IDs may have gaps. Responses are
sorted by ID. A removed ID can be reused by a later new entry. Alexa identities
remain separate and stable for retained entries.
A station entry looks like this:

```json
{
  "id":1,"name":"Base station 1","mac":"AA:BB:CC:DD:EE:01","address_type":1,
  "configured":true,"requested":"on","pending":false,"busy":false,
  "reading":false,"reachable":true,"command_sent":true,"command_confirmed":true,
  "command_status":"confirmed","last_sent_ms":35000,
  "observed":"on","observed_raw":11,"observed_ms":39000,"error":""
}
```

`requested` is only a target and is `null` after reboot. `requested_ms` is its
acceptance time. The legacy field `alexa_target_pending` is always false;
it never changes the local API's `observed`/`observed_raw` measurement.
`observed` is the last BLE measurement. Reads run at startup, after commands and periodically: target 15 s,
starting state 2 s, unreachable state 30 s, subject to the BLE queue.

`reading` includes queued read requests for compatibility. `read_active` means
a worker read is in progress; `read_queued` means a refresh is waiting. Both can
be true if another refresh is requested during a read. The dashboard distinguishes
queued from active and labels the measurement age separately.

A read cycle has up to two worker attempts. NimBLE 2.5.2 allows one internal
retry for HCI 0x3e per connection call, each controller attempt with a 3000 ms
timeout (at most four controller starts per read job). One failed cycle preserves
a previously usable
measurement with `read_failures=1` and a dashboard warning, then retries after
five seconds. Two failed cycles, no previous measurement, or age of at least 60 s
means `reachable=false`. Failures never refresh the measurement timestamp.

`ble_error` is the last numeric NimBLE error, or 0 if unavailable. Code 574
(`0x023e`, HCI `0x3e`) means connection establishment failed, not a specific
GATT read error. Before retrying this code, a two-second passive scan checks the
assigned MAC; configured connections always use Random (1). Recovery scans
never write power or replace the explicit discovery results. Serial output
includes the actual failing phase, attempt and recovery scan signal information.
The scan distinguishes an unseen address from a matching but non-connectable
legacy report, with report type and RSSI. Legacy connectability is checked
explicitly, retaining the workaround for the previous 1.4.3 `isConnectable()` bug.
`last_read_attempt_ms` is the end of the last read attempt; `read_failures` counts
consecutive failed cycles and resets on success. `observed_raw=-1` means no
measurement yet. Times are milliseconds since startup; calculate differences as
unsigned 32-bit values. Runtime state is not persisted across reboots.

### Command acceptance and confirmation

`command_sent` means the power write returned success; it is not proof of the
physical target. `command_confirmed` means a post-write read observed the target
within its confirmation window: raw 11 for ON, raw 0 for SLEEP. Booting values
1/8/9 do not confirm steady ON, and STANDBY (2) does not confirm SLEEP.

`command_status` is one of:

| Value | Meaning |
| --- | --- |
| `none` | No command since boot/reassignment. |
| `queued` | Accepted work is waiting. |
| `sending` | Its worker command attempt is active. |
| `failed` | The bounded write attempts failed. |
| `verifying` | Write accepted, but target not yet confirmed. |
| `confirmed` | Target was read within the window. |
| `unconfirmed` | No target confirmation within 30 seconds after the write. |

An unconfirmed successful write gets read-only checks about every two seconds
for at most 30000 ms from `last_sent_ms`, subject to pending commands and radio
work. No power writes are added. After expiry normal polling continues. A later
measurement still updates physical state, but does not rewrite the expired
command's result. A confirmed result is historical and remains confirmed if an
external controller later changes the station; use `observed` for current state.
New commands reset their confirmation record. Failed writes do not enter this
window.

## POST /api/refresh

Body `{}`, response `202 {"accepted":true}`. Queues reads for all configured entries.
It sends **no power writes**. Follow progress in status.

## POST /api/power

Use `{"station":"all","state":"sleep"}` or `{"station":2,"state":"on"}`.
Only `on` and `sleep` are valid; `off` and `standby` are rejected.
`202 {"accepted":true}` means accepted, not measured. Identical goals already
pending/busy share the active work, generation and original acceptance time.
Opposite targets replace pending requests; an in-flight action may still finish.
A request after completion can queue a new attempt. There are at
most three worker/write attempts. Each connection call allows one internal
HCI 0x3e retry, so there can be up to six controller starts for a command job. All-station requests include only entries with a Bluetooth address;
otherwise the endpoint returns `409 NO_STATION`.

```sh
curl --fail-with-body http://192.168.1.42/api/power \
  -H "X-API-Key: $LH_KEY" -H 'Content-Type: application/json' \
  --data '{"station":"all","state":"sleep"}'
```

## POST /api/scan

Body `{}`, response `202`. Runs a five-second `LHB-…` discovery after queued power
commands. `discovered` contains name, MAC, `address_type` and RSSI, with up to 16
results; `scan_truncated` indicates truncation. Results are never assigned automatically.

## POST /api/config

Replace the complete dynamic list using `{"stations":[…]}` (0–16 entries).
Each entry contains `id`, `name`, `mac` and optional boolean `alexa_compat`:

```json
{"stations":[{"id":1,"name":"Left corner","mac":"AA:BB:CC:DD:EE:01"},{"id":0,"name":"New station","mac":""}]}
```

Keep the existing ID for edits/reordering. Omit `id` or use `0` for a new entry.
The server preserves existing IDs and assigns new entries the lowest free ID
from **1–16**, including ID 5. It reserves all retained IDs before allocating,
regardless of request order. Leave an entry out to remove it and free its slot;
reordering does not change IDs. Responses are sorted by ID and can contain gaps.
Use `id:0`/omit ID for creation, then read the assigned ID from the response.
An ID stays with that entry until removal; a later new entry can reuse it.
The firmware retains server-managed migration identities in stored configuration.
Clients cannot assign them. ID 5 is reserved in that legacy space for the retired
group. Old IDs 6–9 automatically become local 1–4 without changing Alexa keys.
Leave an entry out to remove it. Empty MAC disables an explicitly added entry;
it still counts toward capacity and is not advertised to Alexa. Save returns
`200 {"saved":true,"stations":[{"id":…, "name":…, "mac":…},…]}` with assigned IDs.

`alexa_compat` is a migrated configuration field. It remains a boolean and is
retained when omitted from an existing entry. It has no effect on Matter light
behavior or commissioning. Non-boolean input returns `CONFIG_PROFILE`.

All connections use Random (1); `address_type` input is ignored and status returns
1 for compatibility. Names must be unique, 1–40 UTF-8 bytes, without control
characters or `"`, `\`, `<`, `>`, `&`. Addresses must be unique and are normalized.
Old four-slot records retain their original Alexa identities; empty legacy slots
are discarded while occupied legacy slot IDs are retained. Old pre-0.3.6 dynamic
records without separate Hue IDs migrate to the lowest free slots; 0.3.6 records
retain their saved local IDs. Local IDs, Hue identities and the next Hue
identity counter persist atomically.

Active commands/discovery or maintenance cause `409 BLE_BUSY`. Background reads
do not block saving. State/history follows stable Hue identity and unchanged MAC when reordered;
a changed address resets measurements. Old in-flight reads cannot apply to new
assignments. A malformed list is rejected as a whole (`CONFIG_LIMIT`, `CONFIG_IDS`,
`CONFIG_NAME`, `CONFIG_MAC`); reload after an unknown/removed ID error.

## POST /api/wifi

Body `{}`, response `202 {"accepted":true,"setup_ssid":"Lighthouse-Setup-XXXXXX"}`.
Opens protected Wi-Fi setup and pauses BLE/normal administration HTTP.
Returns `409` during commands, discovery or maintenance. A background read is
allowed to finish before the portal starts. Reboots after completion or a three-minute
timeout. The key and assignments stay unchanged; USB information remains available.

## POST /api/device — web session only

Body `{"name":"vr-room"}`. Changes the setup AP SSID and mDNS hostname together:
`vr-room` and `http://vr-room.local/`. Requires a personal-password web session,
not API-key or bootstrap access. JSON and same-origin rules apply. Name: 1–32
lowercase ASCII letters/digits, optional internal hyphens. Saved in NVS and mDNS
restarted immediately; DHCP hostname is used on subsequent network connection.
Returns `{ "saved":true, "hostname":"vr-room", "setup_ssid":"vr-room", "url":"http://vr-room.local/" }`.
Default legacy names remain until a custom name is saved. Credentials/key unchanged.

## Firmware upload — web session only

Requires a personal-password browser session and a second app
partition. API keys/bootstrap sessions cannot prepare, inspect or upload updates.
The partition table and NVS locations do not change.

1. `POST /api/ota/prepare`, JSON `{}`: pauses new BLE jobs, waits for any current
   background read/cleanup. Commands/discovery/maintenance return `409 BLE_BUSY`.
   Returns `202 {"token":"…","ready":false,"max_bytes":1966080}`. Token is bound
   to the requesting session and only returned here; keep it in memory.
2. Poll `GET /api/ota` until `ready:true`. Response includes `prepared`, `ready`,
   `uploading`, `max_bytes`. A preparation without an upload expires after 60 s.
3. `POST /api/ota`: multipart upload of **one official .lhota file**, with session
   cookie and `X-OTA-Token`. Do not set multipart Content-Type manually; let the
   browser supply its boundary. An Origin header, if present, must match Host.
4. Success: `200 {"installed":true,"restarting":true}`; reboot follows after about
   1.5 s. Sessions are cleared on reboot. No power commands occur at startup.
5. `POST /api/ota/cancel`, JSON `{}`: releases a prepared idle update. An active
   upload or committed installation cannot be cancelled through this endpoint.
   Connection-aborted uploads abort the firmware writer and release BLE.

The 64-byte container header uses magic `LHBRIDGE-ESP32-OTA1` zero-padded to
24 bytes, uint32 little-endian app length at 24, raw SHA-256 digest at 28, and
four zero bytes at 60. The payload must match the board chip and contain the ESP app descriptor
and Lighthouse Bridge product marker. Length, slot capacity, digest, chip/product
and ESP image validity are checked before selecting the inactive slot for boot.
On ESP32-S3, the flash-size field must also match the board profile.
Wrong chip, truncated/corrupt/trailing data, raw/merged BINs and multiple files are
rejected. Package branding/integrity is not a digital signature; use trusted files.
Interrupted uploads leave the previous boot selection intact. No automatic
post-install crash rollback is promised. Wi-Fi/stations/passwords/keys stay in NVS.

Errors: `OTA_INVALID`, `OTA_WRITE_FAILED`, `OTA_ABORTED`, `OTA_UNAVAILABLE`,
`LOGIN_REQUIRED`, `PASSWORD_REQUIRED`, `BLE_BUSY`. A rejected multipart upload is
still consumed by the HTTP library; it cannot write firmware without authorization.

## USB information protocol

Send `LH_INFO\n` over the physical serial port at 115200 baud. The reply is a
single line beginning `LH_INFO `, followed by JSON:

```json
{"protocol":1,"version":"0.1.29","api_key":"<24 hex characters>","setup_ssid":"Lighthouse-Bridge-Setup-XXXXXX","wifi_connected":false,"url":null,"password_set":false}
```

Once connected, `wifi_connected=true` and `url=http://<IPv4>/`.
The personal web password is never included. This information is USB-local; no
unprotected HTTP endpoint exposes it. The installer queries every two seconds
while its USB information reader is connected.

`LH_RESET_WEB_PASSWORD\n` clears only web authentication and sessions, preserving
Wi-Fi, the key and assignments. Recovery requires physical USB access and is never
automatically initiated by the installer.

## Errors

JSON errors have an `error` code; the dashboard translates it into English/German.
`400`: invalid input; `401`: missing/incorrect login or key; `403`: origin;
`409`: busy/no target; `413`: body exceeds 8192 characters; `415`: content type;
`428`: personal password required; `429`: login block; `500`: storage/hash failure.
The request must also fit the endpoint's JSON-document capacity.

Matter 0.3.8 keeps `ble_worker_available:false` until BLE initialization and the
complete Matter endpoint model pass startup. An allocation/model failure leaves
HTTP recovery available with a non-secret `matter_error` / startup phase and radio
requests rejected. `BLE_NO_MEMORY` reports a guarded client-allocation attempt,
without sending power or changing observed readings. The saved station record and
endpoint/fabric identities are not cleared to recover from failed startup.

### rs-matter manual pairing-code renewal

In 0.1.8-rs-matter, personal-session `POST /api/matter/commissioning` with `{}`
generates a fresh code and opens a 15-minute window, replacing any active local
window. Pending duplicate calls coalesce. It returns 202 without a code; poll the
personal-session `GET /api/matter` until `commissioning_pending` is false and
`commissioning_open` is true. `pairing_code` is null while pending/closed/failed or
for an unknown enhanced window. The temporary PIN is never persisted. Existing
bootstrap credentials, fabrics and endpoints stay intact. Nonempty/non-object
JSON is rejected; API keys/bootstrap sessions do not authorize this route.
