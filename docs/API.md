# Local API — 0.3.4

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

The dashboard hides per-station BLE error/retry messages and orange warning
highlighting by default. Regular read phases, last-command history, measurement
ages and unavailable/stale status remain visible. Triple-click its firmware version within 1.2 s
to toggle debug details. This only changes browser presentation; all API fields
and serial diagnostics remain available and BLE behavior is unchanged.

## GET /api/status

Returns version, IP/hostname, Wi-Fi/SSDP initialization, uptime, free heap, a dynamic list of up to 16
entries, discovery state and results. `alexa_started` confirms only SSDP startup.
Top-level fields include `max_stations:16`, `maintenance`, `setup_ssid`,
`ota_supported` and `ota_product`. Station IDs are stable opaque integers.
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
Each entry contains `id`, `name`, `mac`:

```json
{"stations":[{"id":1,"name":"Left corner","mac":"AA:BB:CC:DD:EE:01"},{"id":0,"name":"New station","mac":""}]}
```

Keep the existing ID for edits/reordering. Omit `id` or use `0` for a new entry.
The server allocates stable monotonically increasing IDs, never reusing removed
IDs. ID 5 is permanently reserved for the retired virtual Alexa group. IDs are
opaque identities, not array indexes; do not assume the sequence 1–16.
Leave an entry out to remove it. Empty MAC disables an explicitly added entry;
it still counts toward capacity and is not advertised to Alexa. Save returns
`200 {"saved":true,"stations":[{"id":…, "name":…, "mac":…},…]}` with assigned IDs.

All connections use Random (1); `address_type` input is ignored and status returns
1 for compatibility. Names must be unique, 1–40 UTF-8 bytes, without control
characters or `"`, `\`, `<`, `>`, `&`. Addresses must be unique and are normalized.
Old four-slot records migrate assigned devices to their original IDs 1–4; empty
legacy slots are discarded. The list and next-ID counter persist atomically.

Active commands/discovery or maintenance cause `409 BLE_BUSY`. Background reads
do not block saving. State/history follows ID and unchanged MAC when reordered;
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
Discovery `uniqueid` uses the pinned Espalexa two-digit uppercase hexadecimal
endpoint for IDs 1–255, e.g. `AA:BB:CC:11:22:33-06-00:11`. This restores the
format accidentally changed for new IDs in 0.3.0/0.3.1. Larger monotonic IDs use
`02:<bridge MAC last three octets>:<four station-ID octets>-01`, a fixed EUI-64
and endpoint representation; the numeric light key does not change. Metadata
changes do not assign stations, enqueue commands or modify measurements.

Only individual configured stations appear in the Hue collection. Existing first
four light IDs/unique IDs are preserved. The old virtual group (station ID 5) is
unavailable (Hue error 3), never reused; delete its cached Alexa app entry once.
Use Alexa groups or the local API `station:"all"` for collective control.
Raw starting values 1/8/9 count as active, while the dashboard still says Starting.
There is no state push into the Alexa app: Alexa decides when to query again.

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
Brightness/color/transition operations are not implemented. Bodies are limited
to 512 bytes/eight properties. Existing API-key/web-session endpoint rules remain.

USB logs include `Hue PUT` (accepted target, unsupported count, handler_ms) and
`Hue GET` (reported on, measured reachability, pending work; collection
configured/fresh counts). `source=accepted_target` identifies the bounded target
view; `source=measurement` identifies the measured value.
`station` identifies a persistent station ID; collections log configured/fresh
counts. The retired virtual group is never controlled. These diagnostics
omit usernames, raw request paths/bodies and credentials. Handler time excludes
network transfer/request parsing before dispatch. A correct ACK cannot guarantee
that Alexa will not warn about a later unavailable or unchanged measurement.

## USB information protocol

Send `LH_INFO\n` over the physical serial port at 115200 baud. The reply is a
single line beginning `LH_INFO `, followed by JSON:

```json
{"protocol":1,"version":"0.3.4","api_key":"<24 hex characters>","setup_ssid":"Lighthouse-Setup-XXXXXX","wifi_connected":false,"url":null,"password_set":false}
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
