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
from ota_package import unpack
from build_matter import validate_segments, SEGMENTS, VERSION as MATTER_VERSION
from build_tasmota import VERSION as TASMOTA_VERSION


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
    names = ['bootloader', 'partitions', 'boot-app0', 'app']
    offsets = [0x1000, 0x8000, 0xe000, 0x10000]
    parts = []
    for name, offset in zip(names, offsets):
        file = ROOT / 'dist' / f'lighthouse-bridge-{name}.bin'
        if not file.is_file():
            raise ValueError(f'Missing update segment: {file}')
        end = offset + ((file.stat().st_size + 4095) // 4096) * 4096
        if offset < 0xe000 and end > 0x9000:
            raise ValueError('Update segment would erase NVS')
        if image.read_bytes()[offset:offset + file.stat().st_size] != file.read_bytes():
            raise ValueError('Update segment differs from merged firmware')
        shutil.copy2(file, firmware / file.name)
        # The installer uses normal browser caching: give each image its own URL.
        digest = hashlib.sha256(file.read_bytes()).hexdigest()
        parts.append({'path': 'firmware/' + file.name + '?sha=' + digest, 'offset': offset})
    ota = ROOT / 'dist' / f'lighthouse-bridge-v{VERSION}.lhota'
    if unpack(ota.read_bytes()) != (ROOT / 'dist/lighthouse-bridge-app.bin').read_bytes():
        raise ValueError('OTA payload differs from application')
    shutil.copy2(ota, firmware)
    shutil.copy2(ROOT / 'dist/SHA256SUMS', firmware / 'SHA256SUMS')
    shutil.copy2(ROOT / 'dist' / f'lighthouse-bridge-v{VERSION}.zip', firmware)
    manifest = {
        'name': 'Lighthouse Bridge', 'version': VERSION,
        'new_install_prompt_erase': True,
        'new_install_improv_wait_time': 0,
        'builds': [{'chipFamily': 'ESP32', 'parts': parts}],
    }
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (output / 'build-info.json').write_text(json.dumps({
        'version': VERSION, 'commit': os.environ.get('GITHUB_SHA', ''),
        'sha256': checksum, 'esp_web_tools': upstream['version'],
    }, indent=2) + '\n')
    matter = ROOT / 'dist/matter'
    matter_manifest = json.loads((matter / 'manifest.json').read_text())
    if matter_manifest['version'] != MATTER_VERSION or len(matter_manifest['builds']) != 1 or matter_manifest['builds'][0]['chipFamily'] != 'ESP32':
        raise ValueError('Unexpected Matter prototype identity')
    matter_segments = [(offset, (matter / f'lighthouse-matter-{name}.bin').read_bytes()) for name, offset, _ in SEGMENTS]
    validate_segments(matter_segments)
    if unpack((matter / f'lighthouse-bridge-{MATTER_VERSION}.lhota').read_bytes()) != matter_segments[-1][1]:
        raise ValueError('Matter OTA payload differs from application')
    if matter_segments[1][1] != (ROOT / 'dist/lighthouse-bridge-partitions.bin').read_bytes():
        raise ValueError('Matter prototype changes the stable partition table')
    expected_parts = [{'offset': offset, 'path': f'firmware/matter/lighthouse-matter-{name}.bin?sha=' +
                      hashlib.sha256(data).hexdigest()} for (name, offset, _), (_, data) in zip(SEGMENTS, matter_segments)]
    if matter_manifest['builds'][0]['parts'] != expected_parts:
        raise ValueError('Unsafe Matter manifest or image checksum mismatch')
    matter_output = firmware / 'matter'
    matter_output.mkdir(exist_ok=True)
    for file in matter.iterdir():
        if file.is_file() and file.suffix != '.tmp' and file.name != 'manifest.json':
            shutil.copy2(file, matter_output / file.name)
    shutil.copy2(matter / 'manifest.json', output / 'matter-manifest.json')
    tasmota = ROOT / 'dist/tasmota'
    tasmota_manifest = json.loads((tasmota / 'manifest.json').read_text())
    if tasmota_manifest['version'] != TASMOTA_VERSION or len(tasmota_manifest['builds']) != 1 or tasmota_manifest['builds'][0]['chipFamily'] != 'ESP32':
        raise ValueError('Unexpected Tasmota prototype identity')
    tasmota_segments = [(offset,(tasmota / f'lighthouse-tasmota-{name}.bin').read_bytes()) for name,offset,_ in SEGMENTS]
    validate_segments(tasmota_segments)
    if tasmota_segments[1][1] != (ROOT/'dist/lighthouse-bridge-partitions.bin').read_bytes():
        raise ValueError('Tasmota changes the complete partition table')
    if unpack((tasmota / f'lighthouse-bridge-{TASMOTA_VERSION}.lhota').read_bytes()) != tasmota_segments[-1][1]:
        raise ValueError('Tasmota OTA payload differs from application')
    expected_parts = [{'offset':offset,'path':f'firmware/tasmota/lighthouse-tasmota-{name}.bin?sha='+hashlib.sha256(data).hexdigest()}
                      for (name,offset,_),(_,data) in zip(SEGMENTS,tasmota_segments)]
    if tasmota_manifest['builds'][0]['parts'] != expected_parts:
        raise ValueError('Unsafe Tasmota manifest or checksum mismatch')
    for line in (tasmota/'SHA256SUMS').read_text().splitlines():
        digest,name=line.split()
        if Path(name).name!=name or hashlib.sha256((tasmota/name).read_bytes()).hexdigest()!=digest:
            raise ValueError('Tasmota checksum mismatch')
    tasmota_output = firmware/'tasmota'; tasmota_output.mkdir(exist_ok=True)
    for file in tasmota.iterdir():
        if file.is_file() and file.suffix!='.tmp' and file.name!='manifest.json': shutil.copy2(file,tasmota_output/file.name)
    shutil.copy2(tasmota/'manifest.json',output/'tasmota-manifest.json')
    (output / '.nojekyll').touch()
    print(f'GitHub Pages ready: {output}; stable {VERSION} and experiments {MATTER_VERSION} / {TASMOTA_VERSION}; unchanged slots/NVS; SHA256 {checksum}')


if __name__ == '__main__':
    build()
