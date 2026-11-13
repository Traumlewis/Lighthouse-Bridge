# Version history

Release notes are listed newest first. Current stable firmware: **0.3.9**.
For validation evidence and remaining hardware checks, see [the validation report](docs/VALIDATION.md).

## Experimental Tasmota manual pairing-code renewal 0.1.4-tasmota

The Tasmota dashboard now explicitly generates a new setup code and opens its
15-minute pairing window in one manual action. Each separate activation replaces
the active code; pending duplicate requests coalesce. Use the pinned code generator,
checksum and verifier implementation. Per-window credentials stay in RAM; original
bootstrap credentials, fabrics, endpoints and stations remain intact. Expiry,
closure, failure and reboot invalidate the temporary code. A ready bridge with a
closed window returns no code. Other administrators' enhanced windows never expose
an old local code. No automatic renewal or power command is added.

The authenticated POST requires an empty JSON object. UI clears stale codes before
renewal, polls private status while open/pending and clears timers/state on logout.
Changes to embedded presentation apply only to Tasmota; stable/SDK UI stays intact.
Owner 0.1.3 evidence confirms IPv6/mDNS recovery but shows a closed window, no
commissionable advertisement and zero incoming packets during the new Alexa search.
The new-code workflow still needs device validation; no Alexa success is claimed.

## Experimental Tasmota network lifecycle 0.1.3-tasmota

Reapply the IPv6 request after WiFiManager connects and after observed reconnects;
its hostname setup stops STA and clears Arduino's earlier IPv6 policy. Retry a
missing local address at most every 30 seconds, without changing modem sleep.
Make adapter startup idempotent: the base constructor already starts Matter when
network is available. Refresh delegated A/AAAA addresses on the main loop after
startup/address changes/reconnect, updating existing hosts without removing services.
Preserve identities, credentials, fabrics, commissioning windows and BLE behavior.

Owner 0.1.2 diagnostics reach locally handled CommissioningComplete with one
fabric and no counted receive/send/unacked errors, but Alexa still returns GS014.
One mDNS error precedes traffic and local IPv6 is absent. The corrected lifecycle
issues are verified in source/software; their role in GS014 remains unproven
until a device retest. No automatic reset or pairing-identity replacement is added.

## Experimental Tasmota pairing diagnostics 0.1.2-tasmota

Add bounded, numeric serial diagnostics for Matter commissioning without enabling
raw Tasmota logging. Report packet reception, secure/IM opcodes, commissioning
command masks, transport/errors, locally handled completion, IPv6 presence and
byte heap every 15 seconds. Never log peer identities, packet contents, credentials
or raw exception messages. Counters are cumulative since boot; 0xffff means no
opcode observed. Protocol, identities, fabrics, partitions and BLE policy stay intact.

Owner 0.1.1 evidence confirms startup, discovery of four stations, configuration
restart and one measured/confirmed ON command. Alexa still returns GS014 after
its connection screen on a reported fifth-generation 2022 Echo. The exact failing
commissioning step is not established; this release collects evidence, not a
verified fix. No hardware is attached in development.

## Experimental Tasmota mDNS dependency correction 0.1.1-tasmota

Owner 0.1.0 hardware logs show HTTP recovery with about 72–74 KiB free byte heap,
but Matter fails with import_error. The minimized binary lacks Berry's native
mDNS module because it was tied to disabled generic Tasmota discovery. Restore
that binding through a dedicated flag in the module registry, C module and native
C++ functions. Leave generic discovery/web/MQTT/erase behavior disabled.

Validate mandatory Berry modules in the linked ELF before packaging; the check
reproduces the missing mDNS on the original 0.1.0 ELF. Probe required imports at
boot with non-secret dependency diagnostics. Preserve readiness gating, pairing-code
privacy, BLE behavior, saved identities/fabrics and the exact app slots. The failed
startup log does not prove operational RAM margins or successful Matter/BLE use;
a hardware retest remains required. The working authenticated recovery UI can be
used for the new Tasmota .lhota. Stable and the SDK experiment are unchanged.

## Experimental Tasmota Matter prototype 0.1.0-tasmota

Add a complete, isolated Tasmota/Berry Matter variant for the same classic ESP32.
Reuse production authentication, EN/DE dashboard, 0–16 station editor/discovery,
measured status, local API, physical USB and authenticated product-validated OTA.
A flash-solidified light adapter queues only explicit On/Off/Toggle through the
existing worker; reports remain measured and toggle requires a fresh reading.
No power commands are sent on boot/reconnect, and OFF remains SLEEP.

Keep the complete partition table, both 1.875-MiB slots and shared NVS untouched
by Tasmota management. Disable its web/console/MQTT/OTA/reset/erase/partition-resize
and external startup-script paths. Initialize filesystem only when fully erased;
store Tasmota settings/fabrics there while retaining previous SDK fabrics and the
shared stable endpoint registry/setup credentials. Engine changes need new pairing.

Provide a separate installer, split images, .lhota, checksums and GPL distribution
with the complete modified source/build patches. Add real-host Berry 16-endpoint
composition/command tests, native queue/storage tests and browser/USB simulations.
Software checks do not establish runtime RAM, RF, Alexa pairing, OTA or persistence
on the owner's device. Stable stays 0.3.9; SDK experiment stays 0.3.9-matter.

## Experimental cleanup correction 0.3.9-matter

The owner's 0.3.8 retest still boot-loops. Its exact CI ELF identifies the new
abort in our rollback: SDK endpoint destruction calls Descriptor shutdown although
that cluster was never initialized. Keep explicit initialization history, disable
bare endpoints immediately, and unlink/free uninitialized cluster metadata using
public SDK APIs before destroying the now-empty endpoint. Initialized endpoints
keep normal SDK shutdown. Native tests reproduce the unsafe shutdown contract and
exercise both disposal paths, failures, empty endpoints and light-budget bounds.

Admit another light only with its estimated byte-heap cost plus recovery headroom;
raise that estimate from each completed light. Retain every saved station, endpoint,
fabric and both app slots; do not publish a truncated model. This corrects the
identified cleanup path but does not establish enough RAM for all configured lights.
Real boot/recovery and multi-station operation still require a device test.

## Experimental saved-station startup recovery 0.3.8-matter

Owner evidence confirms 0.3.7 accepts/persists a station save and sends its reply,
then enters a boot loop while building the saved Matter lights. The matching ELF
identifies an allocation-sensitive UART logging lock failure from Matter cluster
creation, plus one failed BLE-client allocation after early worker release.

Initialize the client/scanner-only NimBLE host before Matter and keep the unchanged
worker stack gated until the entire endpoint model is ready. Reserve recovery
memory, prime SDK logging before model allocation and track allocation failures.
Construct the complete light/bridge model with Descriptor clusters before enabling
it; failure releases the reserve and destroys partial RAM endpoints while leaving
station/endpoint/fabric identities in NVS. Keep HTTP/USB/OTA recovery and reject
radio work when startup fails. Guard low-memory BLE client creation. Task stacks,
MTU, packet pools, station capacity, partition table and power behavior are retained.

Exclude private Codex attachments from source archives and package every Matter
header/build requirement. Builds/fake allocation tests do not establish hardware
RAM margins or multi-station success. Stable firmware remains 0.3.9.

## Experimental HTTP/save memory correction 0.3.7-matter

The owner's 0.3.6 discovers four stations without the earlier scan abort, but
HTTP transfers stall/fail and adding stations reports a connection error. Save
handler completion is not established by that excerpt. Replace simultaneous large
save allocations with owned zero-copy input, bounded stack parsing and separate
streamed registry/station persistence. Retain old live assignments until restart
and return assigned fixed IDs plus `restarting:true`; start the full restart grace
period after response serialization. Preserve credentials and all identities.

Check header/body socket writes, retry transient pressure within bounded writer
budgets and give status an exact Content-Length. Serialize browser JSON requests,
pause upload-time polling and quietly reconnect after an acknowledged restart.
Use one simultaneous NimBLE connection for the existing sequential worker; keep
16-station capacity, stacks, MTU and buffer pools. Share the buffered JSON serializer
across counting/HTTP/NVS and use pinned Zopfli for standard gzip UI compression.
Both app slots remain unchanged. Actual hardware memory/save/coexistence/OTA and
multi-station reliability need retesting; stable firmware remains 0.3.9.

## Experimental scan allocation correction 0.3.6-matter

The owner's 0.3.5 discovery abort decodes against the matching release ELF to an
allocation failure while copying NimBLE's advertised-device result vector. Matter
now collects raw legacy GAP reports into fixed worker-owned storage instead of
retaining and copying heap objects for surrounding advertisers. Preserve five-
second active discovery, two-second passive recovery, 16-entry capacity, MAC
matching, Random connections, sequential ownership and read-only behavior.

Parse complete/shortened local names with bounds checks, deduplicate by MAC and
signal capacity overflow. Numeric start/end errors preserve earlier discovery
results. Native tests exercise report floods without C++ allocations and worker
completion/error paths. Real ESP32 discovery, coexistence and memory remain
hardware checks. Stable firmware remains 0.3.9; both app slots are unchanged.

## Experimental status allocation correction 0.3.5-matter

Replace the always-full 4512-byte status snapshot with checked small allocations
only for existing stations and scan results. Preserve every status field and the
coherent measurement/uptime capture; free partial snapshots on failure. Capacity
remains 16 stations/results, without holding the state lock during network IO.
Add byte-addressable data-heap diagnostics; Arduino's internal heap figures can
include word-only IRAM and cannot guarantee that ordinary data allocations fit.
Translate browser timeout/abort failures instead of displaying numeric code 20.

Owner 0.3.4 evidence shows ready startup, one failed and one complete page write,
visible OUT_OF_MEMORY and repeated snapshot allocation failures. This is not a
confirmed leak or a proven hardware fix in 0.3.5. Stable remains 0.3.9; partitions,
credentials, identities, task stacks and power behavior remain unchanged.

## Experimental web delivery diagnostics 0.3.4-matter

Send the compressed dashboard in checked 512-byte socket writes, preserving binary
zeros and Content-Length; stop on incomplete transmission. Log begin/end byte
counts and heap without credentials. Derive default Matter setup/host suffix from
the ESP-IDF STA MAC before the network interface exists; custom names are retained.

Owner evidence now confirms 0.3.3 reaches worker/endpoints/HTTP ready, but the
browser is completely blank. Large TCP writes under low heap are a hypothesis;
0.3.4 needs owner retesting. Stable remains 0.3.9, partitions and settings unchanged.

## Experimental worker startup/recovery 0.3.3-matter

Reserve the unchanged 8-KiB BLE-worker stack before Matter fragments or consumes
heap. Gate the task until BLE initialization completes; startup still reads only
and never sends power commands. Free the temporary configuration text earlier.

Start authenticated HTTP before Matter/BLE initialization. Worker/driver/power-save
failures leave web and firmware recovery available, reject radio requests and
show a specific error. Disable radio controls while keeping firmware upload
available. Report worker availability and safe allocation diagnostics. Matter
commands/code/window readiness also require the worker. Stable remains 0.3.9.

The owner observed 0.3.2 reaching Matter and BLE initialization, then failing to
create the worker before HTTP. Recover that stalled version by USB without erase;
its OTA endpoint is unreachable. Partitions, credentials and station identities
are unchanged. Native/browser failure tests do not establish actual runtime RAM
headroom; a hardware retest remains required.

## Experimental Matter startup correction 0.3.2-matter

Fix endpoint restoration before Matter stack startup. Start the root stack first,
then create or resume the exact saved bridge/light IDs on the CHIP task using the
SDK's restored allocation counter. Enable complete endpoints and report descriptor
changes. Preserve stations, pairing credentials, fabrics and partition layout;
no automatic power command or reset is introduced.

Show Matter startup failures even when there is no code. Remove the renderer's
nested code-element dependency and copy from response state, so replacement of
button contents cannot break rendering or change the copied code. Add an explicit
initialization state and startup-phase diagnostics. The owner's 0.3.1 response
confirmed MATTER_BRIDGE_FAILED, ready=false and a null code. Software regression
checks do not establish successful startup on the owner's ESP32; retest required.
Stable firmware remains 0.3.9.

## Experimental memory correction 0.3.1-matter

Fix the full Matter dashboard's fixed 32-KiB status allocation and additional
whole-response String. Status now streams measured snapshots in 256-byte blocks,
with bounded per-item JSON storage and no network IO under the state lock.
Retain all 16 stations, discovery results, timestamps and measured semantics.
Use configuration/registry JSON capacities sized for their full supported data.

Identify Matter through the authenticated account endpoint, making setup-code
controls available even if general status fails. Preserve personal-password
access and code privacy. Add read-only discovery queue/start/end logs and
non-secret memory diagnostics. The owner's 0.3.0 full-firmware report establishes
the memory error/missing panel; this correction still needs an ESP32 retest.
Both app slots, NVS locations and identities remain unchanged. Stable stays 0.3.9.

## Experimental full firmware 0.3.0-matter

Combine the production dashboard, station editor/discovery, authenticated local
API and streamed Wi-Fi firmware update with up to 16 separate Matter On/Off
Lights. Retain measured status, the one BLE worker, existing settings and exact
flash layout. Stable firmware remains 0.3.9; Matter stays a separate test choice.

The dashboard's **Matter setup** section retrieves the code after a personal web
login and can explicitly open a 15-minute on-network pairing window. API keys
and bootstrap sessions cannot retrieve it. Codes clear on logout and never enter
browser storage or URLs. The USB panel also reads the setup key/web address.
Station saves preserve endpoint identities and restart the bridge without power
writes. Matter owns mDNS; hostname changes preserve its discovery services.

A gzip dashboard, C++ LTO and disabled WiFiManager debug keep the full build in
the original slots. Separate Matter `.lhota` downloads support future dashboard
updates. This combined firmware needs hardware testing; prior single-station
switch/type/status success does not validate the new services or multiple lamps.

## Experimental Matter prototype 0.2.0-matter

Expose all configured stations with nonempty MACs as separate On/Off Lights,
up to the existing capacity of 16. The bridge is named **Lighthouse-Bridge Matter
Test** during testing; the intended final name is **Lighthouse-Bridge**. Saved
station names remain unchanged. The owner confirms the previous single-station
light version switches ON/OFF and displays the correct type/status; this next
multi-station version needs hardware testing.

Persist Matter endpoints by stable Hue identity + MAC, preserving endpoint 1 for
the former single station and aggregator 2. New devices receive monotonically
allocated endpoints; removal/replacement never transfers an old endpoint to a
new physical station. Interrupted registry saves keep survivor identities safe.
The BLE worker is generated from the production source, sharing its bounded
attempts, sequential scheduling, cleanup and measured command confirmation.
The USB panel lists all configured station measurements using their actual IDs.

The same four split flash offsets, both app slots and NVS addresses are retained.
Existing Matter credentials are kept. Alexa may need rediscovery/re-pairing of
only this experimental bridge if it caches the old name or child list; restore
its group/routine assignments if necessary. Keep Erase device unchecked.
Dashboard, HTTP API and Wi-Fi update upload remain the next stage, pending the
multi-station hardware test and further size optimization. Stable remains 0.3.9.

## Experimental Matter prototype 0.1.1-matter

Expose the single bridged station as an **On/Off Light** instead of an On/Off
Plug-in Unit. Version 0.1.0-matter was correctly shown as an outlet because that
was its advertised device type. The owner reports that the initial prototype
works fundamentally on hardware; this does not establish detailed confirmation
timing, external-state synchronization or long-run reliability.

Existing Matter endpoint IDs, unique IDs, commissioning credentials and the
partition layout are preserved. Alexa may retain the previously discovered outlet
classification. If necessary, remove just the prototype's Matter device while the
ESP32 is connected, then pair it again after installing this light-profile update.
Group/routine membership may need restoring. Keep erasing unchecked when updating.
The updated profile needs an owner hardware retest; stable firmware stays 0.3.9.

## Experimental Matter prototype 0.1.0-matter

Add a separate classic-ESP32, single-station Matter experiment to the existing
WebFlasher at `matter.html`. Configure Wi-Fi/stations with stable firmware first;
the prototype uses the assigned station with the lowest local ID. It exposes a
bridged on/off plug endpoint and uses BLE observations for state/reachability.
Matter command acceptance queues ON/SLEEP; it does not invent a measurement.
There are no automatic power commands at boot or reconnect.

The separate build pins Arduino ESP32 3.3.12, NimBLE-Arduino 2.5.2 and ArduinoJson
6.21.5. Both 1.875 MiB app slots and all NVS/partition addresses stay unchanged.
The USB-only installer uses four split images with SHA-256 URLs, plus a separate
prototype ZIP. Pairing information is read over physical USB and kept in browser
memory. The prototype has no dashboard, HTTP API or Wi-Fi update upload; return
to stable 0.3.9 through the stable USB WebFlasher with erasing unchecked.

This is software-tested experimental firmware. No attached ESP32 or Echo validates
flashing, commissioning, runtime memory, NVS migration, radio behavior or app
status. Test attestation credentials may require explicit acceptance by Alexa.

## Version 0.3.9

Add an optional, per-station **Alexa compatibility profile** in the station
settings, off by default. It advertises `Dimmable light` / `LWB010` instead of
`On/off light` / `HASS321`, retaining the station ID and Alexa identity. Test one
station first; after changing its profile, remove that one device from Alexa and
rediscover it to refresh capabilities. Group/routine assignments may need restoring.

The compatibility profile is binary: Hue `bri` 0 means SLEEP, 1–254 means ON,
and acknowledgements/readbacks are coerced to 0/254. Explicit boolean `on` takes
precedence over `bri`; each request queues at most one command. It does not dim
Lighthouse hardware. Standard-profile brightness-only writes remain unsupported.
The local API remains ON/SLEEP-only. The profile is saved with the station and
preserved when an API client omits the optional `alexa_compat` field.

The owner reports that 0.3.8 still returns OFF while the app displays ON, including
after closing the app and clearing its local cache. This alternative device profile
is another targeted compatibility test, not a confirmed fix. Actual Alexa app
behavior and capability refresh require an owner retest. Hue GET logs identify
`profile=standard` or `profile=compat`.

## Version 0.3.8

Alexa status compatibility experiment: Hue GET responses now include
`mode:"homeautomation"` and a read-only binary `bri` hint (0 for the returned
OFF state, 254 for ON/Starting). The hint follows the same bounded accepted-target
view as Hue `on`, then measurements again. It never modifies measurements,
reachability, timestamps or BLE commands. Station IDs, Alexa keys/unique IDs,
`On/off light` type and HASS321 model remain unchanged. Brightness-only writes
remain unsupported; this does not implement physical dimming.

The owner supplied 0.3.7 responses with `on:false` and `reachable:true` while the
Alexa app still showed ON, despite a fresh Hue GET. That rules out missing polling
in that sample but does not establish why the app ignores the update. These extra
fields are a targeted compatibility test, not a confirmed fix. Actual app behavior
needs an owner retest after updating. GET diagnostics now log the returned `bri`.

## Version 0.3.7

Station IDs are **16 fixed slots (1–16)**. A new entry gets the lowest free ID:
first station 1, second 2. Deleting or reordering an entry leaves the other IDs
unchanged, including after reboot. New entries can reuse freed slots. The cards,
setup headers/default names, API and log all use those fixed IDs. Existing 0.3.6
assignments and remaining Alexa identities are preserved. This supersedes the
automatic renumbering introduced in 0.3.6.

## Version 0.3.6

Station numbers now match the configured list everywhere: four entries have IDs
**1–4**, shown as **STATION 01–04**. Local IDs compact after removal or reordering,
up to 16. This also applies to API and worker logs, not only card labels.
Existing IDs such as 6–9 migrate automatically. Alexa keeps a separate stable
identity, so its encoded keys and unique IDs are preserved. No reassignment is
needed. API clients must refresh the station list after configuration changes.

## Version 0.3.5

Restore the quiet normal view: detailed errors, read phases, measurement ages and
last-command history appear only in Debug. Current measured state, unavailability
and queued/sending power commands remain visible. Triple-click the version to
toggle as before. Cards reserve a fixed status area and the footer reserves its
Debug indicator space, so toggling does not move the design. Long diagnostics
scroll inside the card rather than stretching it. BLE/API behavior is unchanged.

## Version 0.3.4

The debug toggle now hides only per-station BLE error/retry messages and orange
warning highlighting. Regular read phases, last-command results, measurement ages
and unavailable/stale status remain visible. Triple-click the firmware version
to toggle warnings, as before. This corrects the overly broad filtering in 0.3.3;
BLE, Alexa and API behavior are unchanged.

## Version 0.3.3

The dashboard hides detailed read/retry errors, measurement ages and last-command
history by default. Triple-click the firmware version within 1.2 seconds to turn
**Debug** on; repeat to turn it off. It resets on reload/sign-out and supports
keyboard activation. Current measured state, unavailable stations and queued or
sending power commands remain visible. API/USB diagnostics and BLE behavior are
unchanged. Install the 0.3.3 .lhota package to update the embedded interface.

## Version 0.3.2

Fix the Hue discovery identifier format for newly added stations. Versions 0.3.0
and 0.3.1 formatted IDs above 4 as unpadded decimal endpoints instead of the
previously working zero-padded hexadecimal representation. This affects the
owner's newly assigned IDs 6–9. Existing first-four identities and numeric Hue
keys are preserved; no station reassignment is needed. Install the 0.3.2 update,
then ask Alexa to discover devices again. Actual Alexa rediscovery needs an owner
retest; the regression and serialized responses are covered by host tests.

## Version 0.3.1

The API key in the ESP32 dashboard now uses the same plain clickable text styling
as the WebFlasher: no visible button background, border or padding. Click or press
Enter to copy. Install the 0.3.1 .lhota package through the dashboard's firmware
update section, or update by USB with Erase device unchecked.

## Version 0.3.0

- Dynamic station list: add/remove up to 16 entries, with stable IDs. Existing
  assigned stations migrate automatically; old empty slots disappear.
- Only individual stations are advertised to Alexa. The old **VR Basisstationen**
  virtual light is retired. Delete that cached device once in the Alexa app;
  create Alexa groups for collective control. Existing individual IDs are kept.
- One Bluetooth-address field accepts manual entry or discovered suggestions.
  All configured stations use Random (1); there is no address-type selector.
- Background reads no longer disable/re-enable the Wi-Fi setup button.
- Click the API key to copy it, in the dashboard or USB WebFlasher panel.
- Custom setup Wi-Fi name, shared with the local `name.local` hostname.
  The setup portal uses the dashboard's dark colors and rounded controls.
- Firmware upload over Wi-Fi using an official **`.lhota`** update package.

The owner has tested four physical stations with earlier firmware, reporting
improved switching and Alexa behavior. The new 16-station capacity and OTA
require hardware verification. Intermittent BLE HCI 0x3e failures remain possible;
this release preserves the bounded transport retries and read-only recovery.

