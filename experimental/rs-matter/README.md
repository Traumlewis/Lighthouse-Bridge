# Lighthouse Bridge — rs-matter prototype 0.1.2-rs-matter

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

Local validation: 1,834,576/1,966,080 app bytes; 66,812 static RAM bytes. Rust
runtime objects allocate separately. Four native Rust tests, fourteen C++ test
programs, 26 Python checks and nine simulated browser/USB programs pass. This is
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
