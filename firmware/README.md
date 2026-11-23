# Lighthouse Bridge firmware

The firmware combines the Arduino ESP32 Wi-Fi and HTTP interfaces, a single
NimBLE worker, and the pinned Rust rs-matter stack. Both 1.875 MiB OTA app slots
and the existing NVS and partition layout are retained across installations.

## Build dependencies

Install Python 3.12, Arduino CLI 1.6.0 and Espressif Rust 1.97.0.0 through
[espup](https://github.com/esp-rs/espup). Install the esp32, esp32c3 and esp32s3
targets and source the export script produced by espup.

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install 'NimBLE-Arduino@2.5.2' 'ArduinoJson@6.21.5' 'WiFiManager@2.0.17'
python -m pip install -r tools/requirements-matter-build.txt
```

From the source root, build the profiles you need. The default is the classic
ESP32. Build all six before assembling Pages.

```sh
python tools/build_rs_matter.py --board esp32
python tools/build_rs_matter.py --board weact-c3
python tools/build_rs_matter.py --board c3-supermini
python tools/build_rs_matter.py --board s3-supermini
python tools/build_rs_matter.py --board xiao-c3
python tools/build_rs_matter.py --board xiao-s3
python tools/build_pages.py
python -m unittest discover -s test -p 'test_*.py'
npm install --no-package-lock
npx playwright install chromium
CHROMIUM_PATH="$(node -p "require('playwright').chromium.executablePath()")" npm test
```

Individual firmware packages are written below `dist/rs-matter/`. Serve
`_site` on localhost to try the WebFlasher. The native and Rust regression
commands are in the repository's firmware workflow.
