#!/usr/bin/env python3
"""Assemble GitHub Pages from an already verified firmware release."""
import base64
import hashlib
import json
import os
from pathlib import Path
import re
import shutil

from package_release import ROOT, VERSION


def build():
    upstream = json.loads((ROOT / 'site/vendor/UPSTREAM.json').read_text())
    for entry in upstream['files']:
        file = ROOT / 'site/vendor' / Path(entry['path']).name
        digest = 'sha256-' + base64.b64encode(hashlib.sha256(file.read_bytes()).digest()).decode()
        if digest != entry['integrity']:
            raise ValueError(f'Vendor integrity mismatch: {file.name}')
    # All relative imports in the bundled modules must resolve locally.
    for file in (ROOT / 'site/vendor').glob('*.js'):
        for relative in re.findall(r'["\x27](\./[^"\x27]+\.js)["\x27]', file.read_text()):
            if not (file.parent / relative).is_file():
                raise ValueError(f'Missing module {relative} imported by {file.name}')
    image = ROOT / 'dist/lighthouse-bridge-esp32-merged.bin'
    checksum = hashlib.sha256(image.read_bytes()).hexdigest()
    recorded = (ROOT / 'dist/SHA256SUMS').read_text().split()[0]
    if checksum != recorded:
        raise ValueError('Firmware checksum mismatch')
    if image.stat().st_size > 4 * 1024 * 1024 or image.read_bytes()[0x1000] != 0xe9:
        raise ValueError('Not a merged 4 MB ESP32 image with bootloader at 0x1000')
    output = ROOT / '_site'
    output.mkdir(exist_ok=True)
    shutil.copytree(ROOT / 'site', output, dirs_exist_ok=True)
    firmware = output / 'firmware'
    firmware.mkdir(exist_ok=True)
    shutil.copy2(image, firmware / image.name)
    shutil.copy2(ROOT / 'dist/SHA256SUMS', firmware / 'SHA256SUMS')
    shutil.copy2(ROOT / 'dist' / f'lighthouse-bridge-v{VERSION}.zip', firmware)
    manifest = {
        'name': 'Lighthouse Bridge', 'version': VERSION,
        'new_install_prompt_erase': True,
        'new_install_improv_wait_time': 0,
        'builds': [{'chipFamily': 'ESP32', 'parts': [{'path': 'firmware/' + image.name, 'offset': 0}]}],
    }
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (output / 'build-info.json').write_text(json.dumps({
        'version': VERSION, 'commit': os.environ.get('GITHUB_SHA', ''),
        'sha256': checksum, 'esp_web_tools': upstream['version'],
    }, indent=2) + '\n')
    (output / '.nojekyll').touch()
    print(f'GitHub Pages ready: {output}; ESP32 manifest, offset 0; SHA256 {checksum}')


if __name__ == '__main__':
    build()
