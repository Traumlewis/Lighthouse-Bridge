# Local API — 0.3.9

The same production HTTP services are also included in experimental
**0.3.8-matter**, with the Matter-only private endpoints below. The separate
[prototype guide](../experimental/matter/README.md) covers installation and
hardware-test limits. Stable firmware remains 0.3.9.

Base address: `http://<ESP32-IP>`. Integrations use `X-API-Key: <key>` as before.
The web dashboard uses an HttpOnly session instead. HTTP Basic has been replaced
by the web login. Mutations require `Content-Type: application/json`. CORS is not enabled.

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

## Experimental Matter setup (personal-password session only)

- `GET /api/matter`: returns `bridge_name`, `version`, `ready`, `paired`,
  `pairing_code` (null until ready), `error`, `commissioning_open`,
  `commissioning_pending`, `commissioning_error` and non-secret `startup_phase`. Use `Cache-Control: no-store`;
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
  station fields retain their production meaning; Matter has no Hue target view
  and returns `alexa_target_pending:false`.
- In the experimental build, station configuration saves checkpoint persistent
  Matter identities, save assignments, pause new BLE work and restart 1.5 seconds
  after responding. Refresh integrations and sign in again after the restart.
  Existing station endpoints survive reordering. Replacing a physical station
  receives a fresh endpoint; no automatic power command is sent on restart.
- The same OTA security/container checks apply. Download the separate Matter
  `.lhota` from the Matter WebFlasher for Matter updates. Boot/fabric persistence
  and actual device uploads still require hardware tests.

## GET /api/status

Returns version, IP/hostname, Wi-Fi/SSDP initialization, uptime, free heap, a dynamic list of up to 16
entries, discovery state and results. `alexa_started` confirms only SSDP startup.
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
acceptance time. `alexa_target_pending` indicates the bounded Hue target view;
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
window. This verification does not prolong the separate Hue accepted-target view.

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
A separate server-managed Hue identity preserves Alexa devices through renumbering;
clients cannot assign it. Only that Hue identity space reserves 5 for the retired
group. Old IDs 6–9 automatically become local 1–4 without changing Alexa keys.
Leave an entry out to remove it. Empty MAC disables an explicitly added entry;
it still counts toward capacity and is not advertised to Alexa. Save returns
`200 {"saved":true,"stations":[{"id":…, "name":…, "mac":…},…]}` with assigned IDs.

`alexa_compat` selects an experimental Alexa LWB010/Dimmable-light profile for
that station only. It defaults to false for old/new records. Omitting it on an
existing entry preserves the current setting; explicit false restores standard.
It cannot change IDs or send power. Non-boolean values return CONFIG_PROFILE.
The flag is included in station status/config responses and stored in NVS.
After changing it, Alexa may need that one device removed/re-discovered to reload
capabilities; its groups/routines may need reassigning. No synchronization fix is
claimed without a successful hardware retest.

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

Requires firmware >=0.3.0, a personal-password browser session and a second app
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
four zero bytes at 60. The payload must be a classic ESP32 app with app descriptor
and Lighthouse Bridge product marker. Length, slot capacity, digest, chip/product
and ESP image validity are checked before selecting the inactive slot for boot.
Wrong chip, truncated/corrupt/trailing data, raw/merged BINs and multiple files are
rejected. Package branding/integrity is not a digital signature; use trusted files.
Interrupted uploads leave the previous boot selection intact. No automatic
post-install crash rollback is promised. Wi-Fi/stations/passwords/keys stay in NVS.

Errors: `OTA_INVALID`, `OTA_WRITE_FAILED`, `OTA_ABORTED`, `OTA_UNAVAILABLE`,
`LOGIN_REQUIRED`, `PASSWORD_REQUIRED`, `BLE_BUSY`. A rejected multipart upload is
still consumed by the HTTP library; it cannot write firmware without authorization.

## Alexa / Hue queries

`/description.xml`, `/api` and `/api/<username>/lights[/<id>[/state]]` are available
to the local Echo without login. GET `on` normally comes from BLE measurements.
While an accepted power command is pending or running, it reports that target
for at most 30000 ms from acceptance. Completion, final error or expiry restores
the measured value; duplicates do not extend the deadline. `reachable` always
uses measured health, including failed cycles and measurement age. Dashboard and
local API measurements are unchanged. This policy was explicitly chosen by the
owner after Echo queried the opposite old value immediately after accepting PUT.
Discovery uses stable Hue identities independent of local/API numbering.
`uniqueid` uses the pinned Espalexa two-digit uppercase hexadecimal
endpoint for Hue identities 1–255, e.g. `AA:BB:CC:11:22:33-06-00:11`. This restores the
format accidentally changed for new IDs in 0.3.0/0.3.1. Larger monotonic IDs use
`02:<bridge MAC last three octets>:<four station-ID octets>-01`, a fixed EUI-64
and endpoint representation; the numeric light key does not change. Metadata
changes do not assign stations, enqueue commands or modify measurements.

Only individual configured stations appear in the Hue collection. Existing first
four light IDs/unique IDs are preserved. The old virtual group (Hue identity 5) is
unavailable (Hue error 3), never reused; delete its cached Alexa app entry once.
Use Alexa groups or the local API `station:"all"` for collective control.
Raw starting values 1/8/9 count as active, while the dashboard still says Starting.
There is no state push into the Alexa app: Alexa decides when to query again.

Since 0.3.8 all Hue GET shapes (collection, light and `/state`) also contain
`bri:0` when the returned `on` is false, otherwise `bri:254`, and
`mode:"homeautomation"`. This read-only binary compatibility hint is not a
physical brightness measurement; ON/Starting share 254. It follows exactly the
same measured/temporarily accepted-target view as `on`, never altering observed
state, measurement age or reachability. Type remains `On/off light` / HASS321.
Standard-profile brightness-only PUTs remain unsupported and cannot send power writes. This is an
experimental client compatibility change; Alexa app synchronization is unverified.

The optional per-station `alexa_compat:true` profile advertises `Dimmable light`
with model `LWB010`, keeping the same unique ID/numeric key. This profile accepts
integer `bri` 0..254: 0 maps to SLEEP; any positive value maps to ON. A valid
explicit boolean `on` takes precedence, including `{on:false,bri:254}`. Queue at
most one power job per request. Success for bri reports the effective binary
0/254 value, not a requested intermediate brightness. Invalid values receive Hue
error 7; unsupported fields error 6. Valid on/bri properties can succeed separately
from invalid/unsupported properties. There is no physical dimming or local-API
brightness endpoint. Standard profile remains strict boolean-on control.

### Hue light writes

`PUT /api/<username>/lights/<id>/state`, body `{"on":true}` or `{"on":false}`,
uses the encoded ID returned by GET, for an individual configured station.
The HTTP handler validates and queues without waiting for BLE. Its HTTP 200
response has the Hue shape (replace `<id>` with the actual requested ID):

```json
[{"success":{"/lights/<id>/state/on":false}}]
```

`false` acknowledges an accepted SLEEP target; `true` acknowledges ON. This is
queue acceptance, not a measured result. Read local status for measurements.
Hue GETs can temporarily project the accepted target as described above; no
measurement or reachability is fabricated. The old generic
`/lights/1/state/` response is no longer used for light state writes.

Malformed JSON, non-boolean `on`, unavailable IDs and unsupported methods return
Hue error arrays, without enqueuing invalid commands. Unsupported properties get
error type 6; a valid `on` in the same body can still succeed independently.
Physical dimming/color/transition operations are not implemented. Bodies are limited
to 512 bytes/eight properties. Existing API-key/web-session endpoint rules remain.

USB logs include `Hue PUT` (accepted target, unsupported count, handler_ms) and
`Hue GET` (reported on, measured reachability, pending work; collection
configured/fresh counts). `source=accepted_target` identifies the bounded target
view; `source=measurement` identifies the measured value.
`station` identifies the current local station ID; collections log configured/fresh
counts. The retired virtual group is never controlled. These diagnostics
omit usernames, raw request paths/bodies and credentials. Handler time excludes
network transfer/request parsing before dispatch. A correct ACK cannot guarantee
that Alexa will not warn about a later unavailable or unchanged measurement.

## USB information protocol

Send `LH_INFO\n` over the physical serial port at 115200 baud. The reply is a
single line beginning `LH_INFO `, followed by JSON:

```json
{"protocol":1,"version":"0.3.9","api_key":"<24 hex characters>","setup_ssid":"Lighthouse-Setup-XXXXXX","wifi_connected":false,"url":null,"password_set":false}
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
