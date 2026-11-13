#!/usr/bin/env python3
"""Build/package the isolated classic-ESP32 Matter prototype, preserving slots/NVS."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import zipfile
from ota_package import package

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.3.9-matter'
CORE = '3.3.12'
FQBN = 'esp32:esp32:esp32:PartitionScheme=min_spiffs'
APP_LIMIT = 0x1e0000
SEGMENTS = [('bootloader', 0x1000, 'MatterBridge.ino.bootloader.bin'),
            ('partitions', 0x8000, 'MatterBridge.ino.partitions.bin'),
            ('boot-app0', 0xe000, 'boot_app0.bin'),
            ('app', 0x10000, 'MatterBridge.ino.bin')]


def validate_segments(segments):
    expected = [0x1000, 0x8000, 0xe000, 0x10000]
    if [offset for offset, _ in segments] != expected:
        raise ValueError('Exactly four classic ESP32 segments required')
    for index, (offset, data) in enumerate(segments):
        end = offset + (len(data) + 4095) // 4096 * 4096
        limit = expected[index + 1] if index < 3 else offset + APP_LIMIT
        if not data or end > limit or (offset < 0xe000 and end > 0x9000):
            raise ValueError('Segment exceeds its slot or erases NVS')
    for index in (0, 3):
        data = segments[index][1]
        if len(data) < 24 or data[0] != 0xe9 or struct.unpack_from('<H', data, 12)[0] != 0:
            raise ValueError('Only classic ESP32 images are allowed')
    table = segments[1][1]
    entries = {}
    for offset in range(0, len(table) - 31, 32):
        if table[offset:offset + 2] != b'\xaaP':
            break
        _, kind, subtype, address, size, label, flags = struct.unpack_from('<HBBII16sI', table, offset)
        entries[label.rstrip(b'\0').decode()] = (kind, subtype, address, size, flags)
    if entries != {
        'nvs': (1, 2, 0x9000, 0x5000, 0), 'otadata': (1, 0, 0xe000, 0x2000, 0),
        'app0': (0, 16, 0x10000, APP_LIMIT, 0), 'app1': (0, 17, 0x1f0000, APP_LIMIT, 0),
        'spiffs': (1, 130, 0x3d0000, 0x30000, 0),
    }:
        raise ValueError('Prototype must keep the existing complete partition layout')


def render_dashboard(version, html_path=None, transform=None):
    html = re.search(r'R"LHHTML\((.*)\)LHHTML"', (ROOT / 'include/web_ui.h').read_text(), re.S)[1]
    html = html.replace('Bridge 0.3.9', 'Bridge ' + version)
    # Matter can be slow under memory/network pressure. A DOMException's numeric
    # AbortError code (20) must not leak through the UI's translated error keys.
    api_end = '}finally{clearTimeout(timer);}'
    if html.count(api_end) != 1:
        raise ValueError('Expected one dashboard API timeout boundary')
    html = html.replace(api_end, "}catch(e){if(typeof e.code==='string')throw e;"
                        "throw Object.assign(new Error(t('NETWORK_ERROR')),{code:'NETWORK_ERROR'});"
                        "}finally{clearTimeout(timer);}")
    save_followup = "  const data=await api('/api/status');statusNeedsReload=false;render(data);"
    if html.count(save_followup) != 1:
        raise ValueError('Expected one configuration-save follow-up')
    html = html.replace(save_followup,
        "  if(result.restarting){clearInterval(pollTimer);if(current){current.maintenance=true;render(current);}"
        "message(()=>t('matterSaved'));waitMatterRestart();return;}\n" + save_followup)
    html = html.replace("if(e.status!==401){$('connection')", "if(e.status!==401&&revision===configRevision&&!configSaving){$('connection')")
    html = html.replace('||polling||configSaving)return;', '||polling||configSaving||otaBusy)return;')
    # Keep only one JSON request in flight against the single HTTP server.
    html = html.replace('loadAccount();const pollTimer=',
        "let apiTail=Promise.resolve();const directApi=api;api=(...args)=>{"
        "const next=apiTail.catch(()=>{}).then(()=>directApi(...args));apiTail=next;return next;};\n"
        "async function waitMatterRestart(){for(let attempt=0;attempt<8;attempt++){"
        "await new Promise(resolve=>setTimeout(resolve,2000));try{await api('/api/account');"
        "location.reload();return;}catch(e){if(e.status===401){location.reload();return;}}}}\n"
        'loadAccount();const pollTimer=')
    if transform is not None: html = transform(html)
    if html_path is not None: html_path.write_text(html)
    try:
        from zopfli.gzip import compress as compress_gzip
    except ImportError:
        raise SystemExit('Install tools/requirements-matter-build.txt using Python 3.12 pip first')
    compressed = compress_gzip(html.encode(), numiterations=15)
    if gzip.decompress(compressed) != html.encode():
        raise ValueError('Compressed dashboard round-trip failed')
    return '#pragma once\nconst unsigned char WEB_UI_GZIP[] PROGMEM = {' + ','.join(str(byte) for byte in compressed) + '};\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--arduino-cli', default='arduino-cli')
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.pio/matter-build')
    parser.add_argument('--skip-compile', action='store_true', help='Package a previously compiled matching sketch')
    args = parser.parse_args()
    library_info = json.loads(subprocess.check_output([args.arduino_cli, 'lib', 'list', '--format', 'json']))
    libraries = {entry['library']['name']: entry['library'] for entry in library_info['installed_libraries']}
    for name, version in [('NimBLE-Arduino', '2.5.2'), ('ArduinoJson', '6.21.5'), ('WiFiManager', '2.0.17')]:
        if libraries.get(name, {}).get('version') != version:
            raise SystemExit(f'Install {name}@{version} using Arduino CLI first')
    sketch = ROOT / '.pio/matter-sketch/MatterBridge'
    sketch.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / 'experimental/matter/MatterBridge/MatterBridge.ino', sketch)
    shutil.copy2(ROOT / 'partitions.csv', sketch)
    (sketch / 'web_ui_gzip.h').write_text(render_dashboard(VERSION,sketch / 'matter-ui.html'))
    if not args.skip_compile:
        # The CLI selects the installed core; refuse silent upgrades/downgrades.
        platforms = json.loads(subprocess.check_output([args.arduino_cli, 'core', 'list', '--format', 'json']))['platforms']
        if not any(p['id'] == 'esp32:esp32' and p.get('installed_version') == CORE for p in platforms):
            raise SystemExit(f'Install esp32:esp32@{CORE} using Arduino CLI first')
        subprocess.run([args.arduino_cli, 'compile', '--fqbn', FQBN, '--build-property',
                        'compiler.cpp.extra_flags=-DWM_NODEBUG -DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED -DCONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED -Oz -flto ' + ' '.join('-I' + str(path) for path in
                          [ROOT / 'include', ROOT / 'src', ROOT / 'experimental/matter/MatterBridge', sketch]),
                        '--build-property', 'compiler.c.extra_flags=-DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED -DCONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED -Oz',
                        '--build-property', 'compiler.c.elf.extra_flags=-flto',
                        '--build-path', str(args.build_dir.resolve()), str(sketch)], check=True)
    segments = [(offset, (args.build_dir / filename).read_bytes()) for _, offset, filename in SEGMENTS]
    validate_segments(segments)
    if (b'LHBRIDGE-RUNTIME-V1-ESP32' not in segments[-1][1] or VERSION.encode() not in segments[-1][1]
            or b'Lighthouse-Bridge Matter Test' not in segments[-1][1]):
        raise ValueError('Application is not this Matter prototype')
    output = ROOT / 'dist/matter'
    output.mkdir(parents=True, exist_ok=True)
    parts, checksums = [], []
    for (name, offset, _), (_, payload) in zip(SEGMENTS, segments):
        filename = f'lighthouse-matter-{name}.bin'
        (output / filename).write_bytes(payload)
        digest = hashlib.sha256(payload).hexdigest()
        checksums.append(f'{digest}  {filename}\n')
        parts.append({'offset': offset, 'path': f'firmware/matter/{filename}?sha={digest}'})
    ota_name = f'lighthouse-bridge-{VERSION}.lhota'
    ota = package(segments[-1][1])
    (output / ota_name).write_bytes(ota)
    checksums.append(f'{hashlib.sha256(ota).hexdigest()}  {ota_name}\n')
    (output / 'SHA256SUMS').write_text(''.join(checksums))
    (output / 'manifest.json').write_text(json.dumps({
        'name': 'Lighthouse Bridge — experimental Matter prototype', 'version': VERSION,
        'new_install_prompt_erase': True, 'new_install_improv_wait_time': 0,
        'builds': [{'chipFamily': 'ESP32', 'parts': parts}],
    }, indent=2) + '\n')
    info = {'version': VERSION, 'arduino_esp32': CORE, 'nimble': '2.5.2',
            'app_bytes': len(segments[-1][1]), 'app_slot_bytes': APP_LIMIT,
            'hardware_validated': False, 'stations': 16, 'bridge_name': 'Lighthouse-Bridge Matter Test',
            'device_type': 'on_off_light', 'dashboard': True, 'local_api': True, 'ota': True}
    (output / 'build-info.json').write_text(json.dumps(info, indent=2) + '\n')
    archive = output / f'lighthouse-bridge-{VERSION}.zip'
    temporary = archive.with_suffix('.zip.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as bundle:
        for file in sorted(output.glob('*.bin')):
            bundle.write(file, file.name)
        bundle.write(output / ota_name, ota_name)
        for name in ['SHA256SUMS', 'build-info.json']:
            bundle.write(output / name, name)
        bundle.write(ROOT / 'experimental/matter/README.md', 'READ-ME-FIRST.md')
        bundle.write(ROOT / 'VERSION.md', 'VERSION.md')
        bundle.write(ROOT / 'LICENSE', 'LICENSE')
        for path in ['experimental/matter/MatterBridge/MatterBridge.ino',
                     *[str(p.relative_to(ROOT)) for p in (ROOT / 'experimental/matter/MatterBridge').glob('*.h')], 'partitions.csv',
                     *[str(p.relative_to(ROOT)) for p in (ROOT / 'include').glob('*.h')],
                     'src/main.cpp', 'tools/build_matter.py', 'tools/ota_package.py', 'tools/requirements-matter-build.txt',
                     'experimental/matter/README.md', 'VERSION.md', 'LICENSE']:
            bundle.write(ROOT / path, 'source/' + path)
        for file in (ROOT / 'third_party').iterdir():
            if file.is_file():
                bundle.write(file, 'third-party-licenses/' + file.name)
                bundle.write(file, 'source/third_party/' + file.name)
        for name in ['NimBLE-Arduino', 'ArduinoJson', 'WiFiManager']:
            directory = Path(libraries[name]['install_dir'])
            for file in directory.rglob('*'):
                if file.is_file() and file.name.lower().startswith(('license', 'notice', 'copying')):
                    bundle.write(file, Path('third-party-licenses') / name / file.relative_to(directory))
    with zipfile.ZipFile(temporary) as bundle:
        if bundle.testzip() is not None:
            raise ValueError('Corrupt prototype ZIP')
    temporary.replace(archive)
    print(f'Matter prototype {VERSION}: {info["app_bytes"]}/{APP_LIMIT} bytes; unchanged slots/NVS; no hardware validation.')


if __name__ == '__main__':
    main()
