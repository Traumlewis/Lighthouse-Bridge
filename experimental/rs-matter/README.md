# Lighthouse Bridge — rs-matter prototype 0.1.5-rs-matter

This replaces the Tasmota experiment with the independent Rust implementation
`project-chip/rs-matter`, pinned to 43f61585a439f1a1e90924b696a6280850ef0145.
The existing production C++ dashboard, authentication, API, OTA receiver and
single NimBLE worker remain shared with stable firmware. No CHIP C++ Matter
library, Tasmota runtime or Berry interpreter is linked into this prototype.

Hardware: classic ESP-WROOM-32 / ESP-32S, at least 4 MB flash. The exact existing
partition table and both 1.875 MiB app slots are mandatory. No merged USB image.
Do not flash firmware for S2/S3/C3 to this board.

Commissioning uses the existing saved Wi-Fi connection, then Matter over IP.
Bluetooth remains dedicated to Lighthouse V2. Commands queue ON (0x01) or SLEEP
(0x00); boot, reconnect, scans and periodic reads never send automatic power.
The Matter state and Reachable attributes use actual measurements. Up to 16
configured stations have distinct bridged On/Off Light endpoints. Their existing
registry under lhmatter/endpoints and custom names are retained.

rs-matter's fabrics, ACLs and other protocol state use a separate lhrsmatter NVS
namespace. Existing Tasmota and CHIP fabric formats are not imported. Switching
protocol implementations requires pairing this prototype with Alexa again; it
must not erase Wi-Fi, the API key, the web password or station assignments. Old
protocol storage is left intact. The test bridge name remains exactly
Lighthouse-Bridge Matter Test. Test attestation is not product certification.

The authenticated dashboard shows the active manual code only on explicit access.
New code & open pairing window generates a new local 15-minute pairing window.
Codes expire with the window and clear on logout/restart. API keys and bootstrap
sessions cannot retrieve them. The setup action does not remove existing fabrics.

Build with the pinned Arduino ESP32 3.3.12 and library versions in
`tools/build_rs_matter.py`, Espressif Rust 1.97.0.0 (espup 0.19.0) and the committed
Cargo.lock. Run `python tools/build_rs_matter.py` after sourcing espup's export.
Outputs are in dist/rs-matter: four split ESP32 images, .lhota, SHA256SUMS and
source ZIP. Future OTA uses only the separate .lhota through a personal-password
web session. Settings-preserving USB updates must leave erasing unchecked.

Build and hardware validation status is recorded in docs/VALIDATION.md. A build
or simulated test does not establish successful Alexa pairing, runtime RAM margins,
actual OTA or prolonged multi-station Bluetooth/Wi-Fi coexistence.

Local validation: 1,836,704/1,966,080 app bytes; 66,924 static RAM bytes. Rust
runtime objects allocate separately. Four native Rust tests, sixteen C++ test
programs, 31 Python checks and nine simulated browser/USB programs pass. This is
not measured runtime memory or an Alexa hardware result. The foreground task has
16 KiB stack and the original BLE task has 8 KiB. Runtime diagnostics show free
heap, largest block and stack watermark for the next hardware test.

Owner hardware evidence for 0.1.0 is negative: repeated cache-error resets occur
before Arduino setup or HTTP. The exact published ELF decodes the failing PC to
Rust's weak flash-resident memmove, called by ESP-IDF read_id_core during flash
initialization. The 0.1.1 link explicitly resolves SDK libc before Rust, retaining its cache/PSRAM
workaround rather than replacing it with fixed ROM addresses. Packaging checks
memcpy, memmove, memset, memcmp and strlen in the final ELF for strong ROM/IRAM
definitions. Regression tests reject the actual failing address and missing,
duplicate, weak and flash definitions. Owner 0.1.1 now confirms physical startup; HTTP and Alexa pairing remain
unverified. Recover 0.1.0 over four-part USB with erasing unchecked; its HTTP
OTA service never starts.

Owner 0.1.1 evidence confirms Matter/BLE ready without the prior cache panic,
but the dashboard is unavailable after WLAN setup. Data heap drops to about
2412 bytes (largest block 2036), later as low as 1652 (largest 916). A later
POWERON_RESET is shown without a panic; it does not establish an OOM-triggered
reboot. The foreground stack watermark is >=18612 of 24576 bytes: observed
startup/idle use is <=5964 bytes. Pairing/OTA peaks were not measured.

0.1.2 releases 16384 bytes of fixed reservation: the ESP32 future arena is
16384 instead of 24576, and the foreground task is 16384 instead of 24576.
The exact operational future (about 12 KiB on Xtensa) is checked at compile time
against its arena, including alignment and 1 KiB reserve. 64-bit host builds use
24576 for their larger pointer-dependent frame. The operational driver still
uses one arena allocation; no SDK source/private layout casts are introduced.
Arena used/capacity are logged. BLE remains 8192 bytes and every protocol/station
capacity stays unchanged. Hardware HTTP delivery, actual OTA, pairing stack peaks
and Alexa interoperability remain unverified for this correction.

Owner 0.1.2 evidence now confirms displayed dashboard, discovery of all four
stations, assignment and successful save. The subsequent configuration restart
and lost session are intentional; retained stations then read SLEEP. Other
POWERON_RESET entries do not establish software crashes. The one explicit panic
has the matching published ELF fingerprint and decodes to newlib
lock_init_generic -> UART write -> vprintf -> Wi-Fi/coexistence/ppTask. The
mutex creation returned null. Heap minima reach 92 bytes and BLE_NO_MEMORY is
shown; later 15-second samples are around 14 KiB with an 11 KiB minimum before
the panic. They do not identify its transient allocation or establish a leak.
No open commissioning window, paired fabric or power command is shown.

0.1.3 warms the shared UART/VFS write lock through public console write before
Wi-Fi starts, while RAM is available. Keep the SDK formatter, log levels and
messages unchanged, including float/long-long formatting.
An allocation-failure hook logs only numeric size/caps/count through ROM, once
per second at most, without reading the SDK function-name string. The periodic
counter includes throttled events. Its non-inlined callback and format/counters
are verified in IRAM/DRAM in the final ELF, including skip-compile packaging;
GCC LTO otherwise lost the inline callback's IRAM section in a local probe.
Native tests cover console warmup/failure and hook registration, an invalid ignored string
pointer, throttling and millis wrap. Hook disassembly calls ROM/IRAM helpers.
Underlying allocation peaks, sustained RAM headroom, actual OTA and physical
stability remain unverified; this is not a general OOM or Alexa pairing fix.

Owner 0.1.3 evidence: no abort/backtrace/panic appears in this excerpt; one
accepted config save has its intended restart, and a later POWERON_RESET does
not prove a software crash. The allocation-failure hook reaches >24,000 events.
Reports include 1528/2308 bytes with INTERNAL|DEFAULT (0x1800) and 442 bytes
with INTERNAL|DMA|8BIT (0x80c); heap minima reach 180 bytes and BLE_NO_MEMORY
recurs. Failures start with the window closed. Later the window opens but the
fabric remains unpaired; the owner says Alexa cannot find the device. This
establishes pressure, not the precise allocator caller or a unique cause of
failed discovery. No raw logs or device identities are retained.

0.1.4 bounds Wi-Fi's public init config after Arduino 3.3.12's hard-coded 32/32
dynamic defaults. This classic-board profile uses TX<=4 and RX<=8, raising the
RX target only when required by existing static RX or Block Ack minima. Already
stricter limits are retained. All unrelated fields, storage/callback/magic data,
static buffer counts and aggregation remain unchanged. A linker wrapper forwards
the copied config once to the real SDK init; no SDK fork/private casts. Startup
logs show actual counts, and final ELF validation requires the wrapper.
https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/WiFiGeneric.cpp
https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32/api-guides/wifi.html
This bounds frame-buffer growth at the cost of peak throughput; actual RAM
headroom and discovery are not established by the build.

Temporary UDP send pressure now yields in 10 ms intervals, with a 1.5 s retry
window; permanent errors, timeout and partial writes return failure. No false
success or partial-datagram resend. Temporary receive errors remain pending.
The real Rust host driver test covers eventual success after two shortages,
hard/short failure, timeout and cancellation with socket reuse. C++ tests cover
public config copy/limits/minima and errno classification. Serial counters show
UDP TX/RX/temporary failures/hard send errors/timeouts, mDNS socket traffic and
reported IPv6 address count, without payloads/addresses/keys. Socket acceptance
is not proof of Echo receipt or successful mDNS parsing. Physical Alexa pairing,
network isolation/IPv6 routing, sustained heap margin and OTA remain open.

Owner 0.1.4 evidence confirms the configured Wi-Fi profile (static RX 4,
dynamic RX 8, dynamic TX 4, Block Ack 6), one IPv6 address, no explicit panic,
and four initial mDNS sends followed by tx=4/rx=0 with mdns_rx=0 throughout the
open window. Heap minima still reach 192/236 bytes and BLE_NO_MEMORY recurs.
Neither queue bounds nor a locally open window establish Echo discovery.

The adapter violated the public NetworkReceive::wait_available contract: a
fixed 5 ms delay falsely signalled packet readiness on empty sockets. The Matter
receiver acquires the shared RX buffer after this call, then blocks in recv_from;
a ready mDNS socket cannot get that buffer. The full-stack regression injects
one standard wire-format _matterc._udp.local PTR question on mDNS while the
Matter socket remains idle. Before the fix it fails with zero consumed packets;
after the fix it consumes the query and emits a Basic Commissioning response.
Earlier probes checked outgoing announcements/code consistency, not inbound
queries, and therefore missed the starvation. This is a reproduced adapter bug;
it does not prove the unique cause of every hardware allocation failure.

0.1.5 probes recv(MSG_PEEK|MSG_DONTWAIT) with one stack byte, preserving the
queued datagram until actual receive. Empty/transient cases wait 5 ms; readable
(including zero-length) returns success and fatal errors fail. No false readiness,
extra heap-backed poll/select object, SDK private-layout access or protocol-capacity
reduction. Native datagram socketpair tests verify empty/ready/repeated peek,
payload retention, zero-length and closed sockets; the same real-Rust host test
checks inbound DNS through the full stack and the existing expiry/power policies.
Actual radio/IPv6 multicast delivery, Alexa pairing, sustained RAM and OTA still
need an owner hardware test. No real network/Echo was used in the host regression.
