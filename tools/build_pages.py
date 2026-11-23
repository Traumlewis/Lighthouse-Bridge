#!/usr/bin/env python3
"""Publish only the verified rs-matter firmware profiles to GitHub Pages."""
import base64
import hashlib
import json
from pathlib import Path
import re
import shutil
from tempfile import TemporaryDirectory

from build_rs_matter import BOARDS, ROOT, SEGMENTS, VERSION
from firmware_assets import validate_segments
from ota_package import unpack


def verify_vendor():
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


def publish_board(output, board_id, board):
    folder = Path('rs-matter')
    if board_id != 'esp32':
        folder /= board_id
    source = ROOT / 'dist' / folder
    destination = output / 'firmware' / folder
    destination.mkdir(parents=True)
    manifest = json.loads((source / 'manifest.json').read_text())
    segments = [
        (board['boot'] if name == 'bootloader' else offset,
         (source / f'lighthouse-rs-matter-{name}.bin').read_bytes())
        for name, offset, _ in SEGMENTS
    ]
    validate_segments(segments, board['chip'], board['boot'], board.get('flash_mib', 4))
    parts = []
    names = []
    for (name, _, _), (offset, data) in zip(SEGMENTS, segments):
        filename = f'lighthouse-rs-matter-{name}.bin'
        names.append(filename)
        digest = hashlib.sha256(data).hexdigest()
        parts.append({'offset': offset, 'path': f'firmware/{folder.as_posix()}/{filename}?sha={digest}'})
    if manifest['version'] != VERSION or manifest['builds'] != [{'chipFamily': board['family'], 'parts': parts}]:
        raise ValueError(f'Invalid manifest for {board_id}')

    ota = f'lighthouse-bridge-{VERSION}.lhota'
    if unpack((source / ota).read_bytes(), board['chip']) != segments[-1][1]:
        raise ValueError(f'OTA payload differs for {board_id}')
    names.append(ota)
    checksums = ''.join(f'{hashlib.sha256((source / name).read_bytes()).hexdigest()}  {name}\n'
                        for name in names)
    if checksums != (source / 'SHA256SUMS').read_text():
        raise ValueError(f'Invalid SHA256SUMS for {board_id}')

    for name in (*names, 'SHA256SUMS', 'build-info.json', f'lighthouse-bridge-{VERSION}.zip'):
        shutil.copy2(source / name, destination)
    manifest_name = 'manifest.json' if board_id == 'esp32' else f'rs-matter-{board_id}-manifest.json'
    shutil.copy2(source / 'manifest.json', output / manifest_name)
    return segments[1][1]


def build():
    verify_vendor()
    with TemporaryDirectory(prefix='.pages-', dir=ROOT) as workspace:
        output = Path(workspace) / 'site'
        shutil.copytree(ROOT / 'site', output)
        partitions = [publish_board(output, board_id, board) for board_id, board in BOARDS.items()]
        if any(table != partitions[0] for table in partitions[1:]):
            raise ValueError('Board partition tables differ')
        published = ROOT / '_site'
        if published.exists():
            shutil.rmtree(published)
        output.rename(published)
    print(f'GitHub Pages ready: {VERSION} for {len(BOARDS)} board profiles')


if __name__ == '__main__':
    build()
