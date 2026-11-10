# Local API — 0.2.6

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
  `password_required` and `setup_ssid`. `api_key` is included only after password
  setup. API-key header authentication does not grant access to this endpoint.
- `POST /api/logout`, body `{}`: removes the web session. The cookie is HttpOnly,
  SameSite=Strict and has a 12-hour lifetime; server sessions also have a 12-hour
  inactivity limit. Reboot discards all sessions.

## GET /api/status

Returns version, IP/hostname, Wi-Fi/SSDP initialization, uptime, free heap, four
slots, discovery state and results. `alexa_started` confirms only SSDP startup.
A station entry looks like this:

```json
{
  "id":1,"name":"Base station 1","mac":"AA:BB:CC:DD:EE:01","address_type":0,
  "configured":true,"requested":"on","pending":false,"busy":false,
  "reading":false,"reachable":true,"command_sent":true,"last_sent_ms":35000,
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
assigned MAC and refreshes its address type for that job only. Recovery scans
never write power or replace the explicit discovery results. Serial output
includes the actual failing phase, attempt and recovery scan signal information.
The scan distinguishes an unseen address from a matching but non-connectable
legacy report, with report type and RSSI. Legacy connectability is checked
explicitly, retaining the workaround for the previous 1.4.3 `isConnectable()` bug.
`last_read_attempt_ms` is the end of the last read attempt; `read_failures` counts
consecutive failed cycles and resets on success. `observed_raw=-1` means no
measurement yet. Times are milliseconds since startup; calculate differences as
unsigned 32-bit values. Runtime state is not persisted across reboots.

## POST /api/refresh

Body `{}`, response `202 {"accepted":true}`. Queues reads for all configured slots.
It sends **no power writes**. Follow progress in status.

## POST /api/power

Use `{"station":"all","state":"sleep"}` or `{"station":2,"state":"on"}`.
Only `on` and `sleep` are valid; `off` and `standby` are rejected.
`202 {"accepted":true}` means accepted, not measured. Identical goals already
pending/busy share the active work, generation and original acceptance time.
Opposite targets replace pending requests; an in-flight action may still finish.
A request after completion can queue a new attempt. There are at
most three worker/write attempts. Each connection call allows one internal
HCI 0x3e retry, so there can be up to six controller starts for a command job. Groups include only assigned slots;
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

Save all four slots together as `{"stations":[…]}`. Each entry has the shape
`{"name":"Base station 1","mac":"AA:BB:CC:DD:EE:01","address_type":0}`.
An empty MAC disables the slot. Type `0` means public, `1` random; preserve the
discovered type. Names must be unique, 1–40 UTF-8 bytes, without control characters
or `"`, `\`, `<`, `>`, `&`. `VR Basisstationen` is reserved. MACs are unique and
normalized. Success: `200 {"saved":true}`. Active commands or discovery cause
`409 BLE_BUSY`; changes are atomic. Changing an address clears its previous
readings. Background reads do not block saving; results for the previous
assignment are discarded.

## POST /api/wifi

Body `{}`, response `202 {"accepted":true,"setup_ssid":"Lighthouse-Setup-XXXXXX"}`.
Opens protected Wi-Fi setup and pauses BLE/normal administration HTTP.
Returns `409` during active BLE work. Reboots after completion or a three-minute
timeout. The key and assignments stay unchanged; USB information remains available.

## Alexa / Hue queries

`/description.xml`, `/api` and `/api/<username>/lights[/<id>[/state]]` are available
to the local Echo without login. GET `on` normally comes from BLE measurements.
While an accepted power command is pending or running, it reports that target
for at most 30000 ms from acceptance. Completion, final error or expiry restores
the measured value; duplicates do not extend the deadline. `reachable` always
uses measured health, including failed cycles and measurement age. Dashboard and
local API measurements are unchanged. This policy was explicitly chosen by the
owner after Echo queried the opposite old value immediately after accepting PUT.
Light IDs
remain compatible with the previous Espalexa scheme. Group ON requires every
assigned slot's Hue view to be ON (active target or measured); group reachability
requires usable readings for all.
Raw starting values 1/8/9 count as active, while the dashboard still says Starting.
There is no state push into the Alexa app: Alexa decides when to query again.

### Hue light writes

`PUT /api/<username>/lights/<id>/state`, body `{"on":true}` or `{"on":false}`,
uses the encoded ID returned by GET, including the existing virtual group ID.
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
`station=0` means the virtual group, 1–4 the individual slots. These diagnostics
omit usernames, raw request paths/bodies and credentials. Handler time excludes
network transfer/request parsing before dispatch. A correct ACK cannot guarantee
that Alexa will not warn about a later unavailable or unchanged measurement.

## USB information protocol

Send `LH_INFO\n` over the physical serial port at 115200 baud. The reply is a
single line beginning `LH_INFO `, followed by JSON:

```json
{"protocol":1,"version":"0.2.6","api_key":"<24 hex characters>","setup_ssid":"Lighthouse-Setup-XXXXXX","wifi_connected":false,"url":null,"password_set":false}
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
`409`: busy/no target; `413`: body exceeds 2400 characters; `415`: content type;
`428`: personal password required; `429`: login block; `500`: storage/hash failure.
The request must also fit the endpoint's JSON-document capacity.
