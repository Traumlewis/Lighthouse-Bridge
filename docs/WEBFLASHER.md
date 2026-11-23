# WebFlasher

Open the [Lighthouse Bridge WebFlasher](https://traumlewis.github.io/Lighthouse-Bridge/)
in Chrome or Edge on a computer. It offers the same rs-matter firmware for six
board profiles. Select the exact board before installing or downloading a
firmware package. Leave **Erase device** unchecked to keep settings.

The WebFlasher uses four USB segments at board-specific offsets. The partition
table, both 1.875 MiB app slots and NVS addresses match across the profiles.
For subsequent Wi-Fi updates, upload the selected board's `.lhota` through
the dashboard. The ZIP includes USB segments, OTA, checksums, source and
dependency license notices.

Close the installer to let the USB panel read the setup key and address.
With LH-Bridge 0.1.33 or later, the panel shows the setup portal link while it
is active. Open it after connecting to the setup Wi-Fi. After joining the home
network, the panel shows the bridge's dashboard address.
The same connection can read Matter commissioning status via
`LH_INFO` and `LH_MATTER_INFO`. Keys and pairing codes stay in page memory.
You can also retrieve the Matter code from the dashboard after signing in with
your own password.

The site is assembled in a fresh directory so obsolete installers and binaries
disappear when the GitHub Pages workflow publishes it. ESP Web Tools is served from
the checked-in vendor bundle. Its integrity is verified at build time.

See the [main guide](../README.md) for initial Wi-Fi setup and
[firmware build instructions](../firmware/README.md) for local builds.
