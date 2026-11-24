# Lighthouse Bridge — project instructions

Read this file before edits. Update it in the same change when requirements,
hardware, architecture, auth, flashing, workflows or verification status
change. Keep current facts. User instructions win.

## Scope and device authorization

- Work on `main` in `Traumlewis/Lighthouse-Bridge`. The owner authorized the
  consolidation to rs-matter, cleanup of legacy builds and WebFlashers, and
  publication through GitHub Pages. Preserve Git history.
- Ask explicitly before any device change, including firmware writes, restart,
  factory reset and commissioning. Previous approvals cover only the specified
  actions, including the 0.1.29 OTA. Credentials do not grant standing approval.
  Read-only checks may proceed.
- One firmware family, rs-matter, with six WebFlasher profiles. Classic
  ESP-WROOM-32 / ESP-32S with at least 4 MB flash and CH340 USB-C is the owner’s
  board. Also build WeAct ESP32-C3FH4, C3 SuperMini, S3 SuperMini, and Seeed
  Studio XIAO ESP32-C3 and ESP32-S3. The SuperMini S3 uses 4 MB flash and 2 MB
  QSPI PSRAM. The standard XIAO S3 uses 8 MB flash and 8 MB OPI PSRAM; the
  same partition table uses its first 4 MB. Keep the chip and S3 flash
  capacity checks in OTA. The XIAO builds have board-specific GPIO allowlists
  for exposed pins and USB CDC on boot. An owner C3 booted 0.1.30 on home Wi-Fi
  and discovered four stations, with severe heap pressure and truncated
  dashboard transfers. Full C3/S3 hardware verification is pending.
- The owner’s classic ESP32 runs 0.1.29 with four reachable stations and
  persistent Alexa Matter pairing. Owner-approved OTA from the observed
  0.1.28-rs-matter baseline succeeded with one upload. A live browser check
  verified the new label. The device returned four serials as text in 0.1.26.
  One 0.1.27 Dangerzone restart was confirmed by uptime. The served dashboard
  reloaded after a simulated reconnect. No physical reset, GPIO wake or
  Identify LED behavior is verified. Other Matter platforms are listed as
  compatible-controller options, with Alexa the one tested.
- The bridge supports up to 16 stations. No additional phone app or
  smartwatch-specific workflows. No Improv Serial.

## Build and release

- `firmware/` contains the Arduino entry point and no_std Rust library.
  `src/main.cpp` owns the worker and HTTP workflow. `src/dashboard.html` is the
  dashboard source, with its firmware version substituted at build time and
  embedded as gzip. `include/` contains shared firmware headers.
- Starting with 0.1.29, the WebFlasher, dashboard, boot banner and Matter
  software version string show `LH-Bridge <version>`. Manifest, API and USB
  protocol version fields use the numeric version without a product suffix.
  The USB reader also accepts the previous suffixed Matter versions.
  Actions run 45140983557 published 0.1.29. All four live artifact checks
  and EN/DE WebFlasher checks at 1040, 390 and 320 px passed.
- Pin Arduino ESP32 3.3.12, NimBLE-Arduino 2.5.2, WiFiManager 2.0.17,
  ArduinoJson 6.21.5, Espressif Rust 1.97.0.0, espup 0.19.0 and rs-matter rev
  `43f61585a439f1a1e90924b696a6280850ef0145`. Keep Cargo resolution locked.
- Preserve the complete partition table including both 1.875 MiB app slots,
  NVS and OTA addresses. USB flash parts are bootloader at 0x1000 on classic or
  0x0 on C3/S3, partition table at 0x8000, boot-app0 at 0xe000 and app at
  0x10000. Match Rust targets, board chip IDs, offsets, USB and OTA manifests.
  Require checksum, app marker, correct chip and capacity. Never erase standard
  NVS except after an explicitly confirmed factory reset.
- `tools/build_rs_matter.py` emits per-board USB segments, verified app-only
  OTA, checksums, and source ZIP with license notices. Keep stale fingerprints,
  unsafe chip/offsets and invalid OTA payloads out. Never package credentials,
  raw private diagnostics, attachments or Rust target directories.
- Resolve cache-safe ESP-IDF libc before Rust and keep the Rust static library
  inside the rescan group. Preserve ROM/IRAM guards for memcpy, memmove,
  memset, memcmp and strlen, diagnostic symbol guards, Wi-Fi init wrapper,
  pinned stack and arena sizes, host-only NimBLE configuration, and C3
  critical-section portable atomics. Fast paths must fit both OTA slots.
- The C3 Rust release build disables the LLVM machine outliner. Reject
  linked C3 images containing machine-outlined functions. Keep the C3
  SDK log level at errors for slot capacity, with typed
  state, endpoint, network and heap traces preserved.
  Actions run 45281600753 published 0.1.36. All six live packages passed;
  all three C3 release ELFs are free of machine-outlined functions.
  The Alexa commissioning result on C3 still needs a hardware check.
  The owner reported the temporary return to 0.1.29 did not help and
  requested 0.1.36 as the active source and WebFlasher baseline again.
  Actions run 45290565133 republished the original 0.1.36 Pages artifact.
- `tools/build_pages.py` assembles a clean site with the six boards and the
  vendored ESP Web Tools bundle. The project root is the main WebFlasher.
  Retire the old installer URLs. The default classic manifest is
  `manifest.json`; board-specific manifests keep their separate names.
  The main-branch workflow builds, checks and deploys Pages. Verify live
  manifests, chip headers, partitions, OTA comparisons and hashes after deploy.
  Keep English/German, safe consent gating, board selection and USB single ownership.
- Release 0.1.30 adds the two XIAO profiles and is published from commit
  663a438 (Actions run 45163198385). All six live manifest, chip, flash,
  partition, OTA, checksum and ZIP checks passed. Live EN/DE board selection
  and consent passed at 1040, 390 and 320 px. XIAO hardware remains untested.
- The owner confirmed that 0.1.30 works after disabling the VPN and chose it
  as the active development baseline. Restore the source and six WebFlasher
  profiles from 663a438. The temporary test page from Actions run 45254080953
  is superseded by the root WebFlasher. Actions run 45255471898 redeployed
  the exact original 0.1.30 artifact. Live verification matched all six
  manifests, USB segment hashes, OTA packages and ZIPs to the original.
- 0.1.28-rs-matter is published from commit 3c6c098 (Actions run 45124743206).
  All four live manifests, chip/offset checks, partition tables, OTA payloads,
  hashes and source ZIPs passed. Live EN desktop and DE mobile checks passed
  for selection, downloads and consent.
- Actions run 45127780654 published removal of the old rs-matter installer
  URL. The old page and redirect script return HTTP 404. The root installer
  and all four manifests remain available.
- The WebFlasher groups board installation and downloads separately from USB
  setup in a responsive layout. The repository README links to the main
  WebFlasher and shows a screenshot generated from it. Local EN/DE layout
  checks at 1040, 390 and 320 px and browser board-selection/USB checks pass.
  Actions run 45133612733 published the layout. The live site passed EN/DE
  checks at the same three widths, including board selection and consent.

## BLE and station data

- The single worker scans, connects, reads, writes and disconnects
  sequentially. HTTP stays responsive. Never send automatic power commands
  on boot or reconnect. Startup/periodic reads are intentional.
- Lighthouse V2 uses control service `00001523-1212-efde-1523-785feabcd124`
  and power characteristic `00001525-1212-efde-1523-785feabcd124`.
  Use Random (1) addressing. ON is 0x01, OFF defaults to SLEEP 0x00 and
  may be configured per station as STANDBY 0x02. Validate unique MACs/names.
- Use fixed local IDs 1–16 and allocate the lowest free one. Reserve retained
  IDs first, sort by ID and never renumber survivors. Keep persisted
  `hue_id` and `next_id` migration fields and their existing identity semantics.
  Preserve separate Matter endpoint IDs and never reuse retired endpoints.
  Generation checks prevent stale BLE results from applying after reassignment.
- A goal is separate from a measurement. Current state always uses observed
  raw values/timestamps and measured reachability. 11 is steady ON, 0 Sleep,
  2 Standby and 1/8/9 Starting. One read failure retains a recent reading and
  retries after 5 s. Two failures, no reading or age >=60 s means unavailable.
  Never grant grace to an unmeasured post-write state. Background reads run
  about every 15 s, faster during startup/verification and slower when unreachable.
- Coalesce identical active goals and supersede old jobs with a new target.
  At most three command and two read worker attempts, with one internal
  connect retry per call and a 3 s connect timeout. On HCI 0x3e only, use a
  2 s passive recovery scan matched by saved MAC, using explicit legacy
  connectable report types. Retry the assigned address even when unseen.
  Keep physical BLE-active true through asynchronous cleanup and wait until
  the created-client count is zero before another job or Wi-Fi setup.
- Confirm command delivery with separate read-only checks for at most 30 s
  from last send. Confirm only matching steady ON, Sleep or Standby. Keep
  command history latched independently of later state. Never auto-resend on
  verification failure. Never log credentials or raw private responses.
- Identify is an explicit personal-session V2 action bound to the saved
  station and MAC. Write the Identify characteristic only. The command may
  wake the station. Device information uses the same sequential worker for
  read-only type/name/channel and Bluetooth Device Information Service
  reads. Preserve firmware line breaks, printable serials as text and binary
  numbers as big-endian. Missing attributes are unavailable, and results are
  private/transient.

## Matter, network and privacy

- Keep Rust FFI and poll on the foreground Arduino task. Never poll from BLE,
  interrupts or reentrantly, and hold no state lock during network IO. Rust
  owns mDNS/DNS-SD, fabrics and ACLs in `lhrsmatter` NVS. Preserve the
  `lhmatter/endpoints` registry, aggregator 2 and authorized unbound-endpoint-1
  exception. Keep metadata sorted by actual endpoint ID and retain direct
  and wildcard Descriptor regression coverage. Power commands enqueue physical
  work. Matter OnOff and Reachable report measurements.
- Commission over saved home Wi-Fi and DNS-SD only. No window on startup.
  Explicit personal-session renewal opens a fresh 15-minute Basic window
  after Matter/BLE readiness. Coalesce pending requests, bind the pairing code
  to the active window and clear it on close, failure, logout or restart.
  Opening the panel reads status without opening a window. Alexa is verified.
- On C3, explicit commissioning reserves up to 60 seconds without
  background BLE reads or scans. Finish active BLE work before opening
  when possible, with a six-second bound. Power and command verification
  retain priority, and reads report measured freshness. The dashboard
  skips automatic status polling during this interval. Record redacted
  heap, DMA block, stack and failure samples around this interval.
  Actions run 45292614164 published 0.1.37 and all six live packages passed.
  The owner confirmed Alexa commissioning on a C3 SuperMini with 0.1.37.
  The fabric survived a USB restart, and Matter ON/SLEEP were confirmed by
  readings for all four stations. Heap allocation failures still occurred.
  A subsequent 0.1.37 log kept its Matter fabric but showed two truncated
  full dashboard responses during sustained heap pressure.
  Keep eight subscriptions, two responders and existing stack reservations.
- Keep Wi-Fi modem sleep enabled with WIFI_PS_MIN_MODEM before BLE and after
  reconnect. Seed the Matter network store from saved Wi-Fi. Preserve IPv6
  policy, custom hostname, AP and DNS-SD behavior.
- Default setup SSID and hostname are `Lighthouse-Bridge-Setup-XXXXXX` and
  `lighthouse-bridge-xxxxxx` using the STA MAC suffix. Valid custom names take
  precedence. WiFiManager runs a protected nonblocking setup portal with
  USB diagnostics. BOOT is GPIO9 on C3 and GPIO0 on classic/S3.
- Release 0.1.34 uses the 0.1.30 WiFiManager auto-connect and portal
  path. The setup experiments in 0.1.31 through 0.1.33 are historical.
- On C3, the single-client NimBLE host uses 12 MSYS2 blocks and 12 incoming
  HCI ACL blocks. Preserve their block sizes, controller and advertising event
  configuration. This releases fixed heap reservations for HTTP and Wi-Fi.
  Actions run 45260071299 published 0.1.34. All six live profiles and firmware
  packages passed verification. C3 login and BLE behavior still need a
  hardware check with 0.1.34.
- With four stations on C3, 0.1.34 saved configuration and restarted as
  intended. Full dashboard transfers then stalled during BLE reads.
  From 0.1.35 the dashboard returns to sign-in in the open page after a
  station save and uses the public login-options version to detect a
  firmware change. On C3 full dashboard transfers reserve a brief BLE
  scheduling window, drain active work, and release it on every exit.
  Actions run 45267392693 published and verified all six profiles. The
  simulated save/restart flow passed. Physical C3 verification is pending.
  An owner-approved C3 SuperMini OTA from 0.1.34 was rejected with
  `OTA_WRITE_FAILED`; a paced retry also failed. Read-only verification
  afterward found 0.1.34 still running and all four stations reachable.
- Keep the 24-hex API key in NVS as the setup Wi-Fi password and integration
  key. First sign-in uses the key, then forces an own password. Store the
  salted PBKDF2-SHA256 hash, preserve optional username and session gates,
  and exclude key/name from public login options. Only show the USB sign-in
  hint before a personal password is set. Keep USB keys and codes in memory.
- Keep Home Wi-Fi and Local Wi-Fi independent of Remote Mode. Local Mode
  offers direct AP access without Matter. Active-low GPIO buttons use board
  allowlists, stable release, 40 ms debounce and one action per press.
  Remote Mode requires valid saved wake bindings for All on/All off and a
  station. Branch before Wi-Fi/HTTP/Matter, wake only on buttons, complete
  queued commands plus BLE cleanup and confirmation, then deep sleep.
  Two held for five seconds exit Remote Mode without sending power.
- The room label is private, editable and independent of network naming.
  Dangerzone follows account settings. Restart pauses new work, waits on
  BLE idle and reloads the same URL after sign-in becomes reachable. For a
  factory reset only, show a new random session-bound key before confirmation.
  The preview expires in five minutes. An exact confirmed key is written
  with the durable reset flag and reinstalled after erasing standard NVS
  before normal startup. Do not reset OTA selection or app slots. Keep the
  new setup credentials available on the old page after reset acceptance.
