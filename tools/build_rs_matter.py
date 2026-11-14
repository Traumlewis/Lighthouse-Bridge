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
from rs_matter_link import validate_cache_safe_symbols

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.1.1-rs-matter'
RS_REV = '43f61585a439f1a1e90924b696a6280850ef0145'
from matter_dashboard import rotation_dashboard
CORE = '3.3.12'
FQBN = 'esp32:esp32:esp32:PartitionScheme=min_spiffs'
APP_LIMIT = 0x1e0000
SEGMENTS = [('bootloader', 0x1000, 'RsMatter.ino.bootloader.bin'),
            ('partitions', 0x8000, 'RsMatter.ino.partitions.bin'),
            ('boot-app0', 0xe000, 'boot_app0.bin'),
            ('app', 0x10000, 'RsMatter.ino.bin')]


from build_matter import validate_segments, render_dashboard

def source_fingerprint():
    paths=[ROOT/'src/main.cpp',ROOT/'include/web_ui.h',ROOT/'partitions.csv',
           ROOT/'tools/build_rs_matter.py',ROOT/'tools/build_matter.py',ROOT/'tools/matter_dashboard.py',ROOT/'tools/rs_matter_link.py']
    for directory,patterns in [('include',['*.h']),('experimental/matter/MatterBridge',['*.h']),('experimental/rs-matter',['*.h','*.cpp','*.ino']),('experimental/rs-matter/rust',['Cargo.toml','Cargo.lock']),('experimental/rs-matter/rust/src',['*.rs'])]:
        for pattern in patterns:paths.extend((ROOT/directory).glob(pattern))
    digest=hashlib.sha256()
    for path in sorted(set(paths)):
        digest.update(str(path.relative_to(ROOT)).encode());digest.update(path.read_bytes())
    return digest.hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--arduino-cli', default='arduino-cli')
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.pio/rs-matter-build')
    parser.add_argument('--skip-compile', action='store_true', help='Package a previously compiled matching sketch')
    args = parser.parse_args()
    library_info = json.loads(subprocess.check_output([args.arduino_cli, 'lib', 'list', '--format', 'json']))
    libraries = {entry['library']['name']: entry['library'] for entry in library_info['installed_libraries']}
    for name, version in [('NimBLE-Arduino', '2.5.2'), ('ArduinoJson', '6.21.5'), ('WiFiManager', '2.0.17')]:
        if libraries.get(name, {}).get('version') != version:
            raise SystemExit(f'Install {name}@{version} using Arduino CLI first')
    sketch = ROOT / '.pio/rs-matter-sketch/RsMatter'
    sketch.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / 'experimental/rs-matter/RsMatter.ino', sketch)
    shutil.copy2(ROOT / 'partitions.csv', sketch)
    (sketch / 'web_ui_gzip.h').write_text(render_dashboard(VERSION,sketch / 'matter-ui.html',rotation_dashboard))
    shutil.copy2(ROOT/'experimental/rs-matter/rs_platform.cpp',sketch/'rs_platform.cpp')
    crate=ROOT/'experimental/rs-matter/rust'
    rustlib=crate/'target/xtensa-esp32-none-elf/release/liblighthouse_rs_matter.a'
    fingerprint=source_fingerprint()
    stamp=args.build_dir/'lighthouse-source-sha256'
    if args.skip_compile and (not stamp.exists() or stamp.read_text().strip()!=fingerprint):
        raise ValueError('Compiled rs-matter source fingerprint does not match; rebuild required')
    if not args.skip_compile:
        version=subprocess.check_output(['rustc','+esp','--version'],text=True)
        if not version.startswith('rustc 1.97.0'): raise ValueError('Espressif Rust 1.97.0.0 required')
        subprocess.run(['cargo','+esp','build','--locked','-Z','build-std=core,alloc','--target','xtensa-esp32-none-elf','--release','--manifest-path',str(crate/'Cargo.toml')],check=True)
        # The CLI selects the installed core; refuse silent upgrades/downgrades.
        platforms = json.loads(subprocess.check_output([args.arduino_cli, 'core', 'list', '--format', 'json']))['platforms']
        if not any(p['id'] == 'esp32:esp32' and p.get('installed_version') == CORE for p in platforms):
            raise SystemExit(f'Install esp32:esp32@{CORE} using Arduino CLI first')
        # Resolve SDK archives first, then Rust inside the same rescan group.
        # Whole-archive before the group overrides cache-safe libc with weak
        # compiler_builtins and crashes during flash initialization.
        subprocess.run([args.arduino_cli, 'compile', '--fqbn', FQBN, '--build-property',
                        'compiler.cpp.extra_flags=-DWM_NODEBUG -DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED -DCONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED -Oz -flto ' + ' '.join('-I' + str(path) for path in
                          [ROOT / 'include', ROOT / 'src', ROOT / 'experimental/matter/MatterBridge', ROOT/'experimental/rs-matter', sketch]),
                        '--build-property', 'compiler.c.extra_flags=-DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED -DCONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED -Oz',
                        '--build-property', 'compiler.c.elf.extra_flags=-flto',
                        '--build-property', 'compiler.libraries.ldflags=-Wl,--undefined=memcpy,--undefined=memmove,--undefined=memset,--undefined=memcmp,--undefined=strlen -lc '+str(rustlib),
                        '--build-path', str(args.build_dir.resolve()), str(sketch)], check=True)
    properties = subprocess.check_output([args.arduino_cli, 'compile', '--fqbn', FQBN,
                                          '--show-properties=expanded', str(sketch)], text=True)
    compiler_path = next(line.split('=', 1)[1] for line in properties.splitlines()
                         if line.startswith('compiler.path='))
    nm = Path(compiler_path) / 'xtensa-esp-elf-nm'
    cache_symbols = validate_cache_safe_symbols(subprocess.check_output(
        [str(nm), '--defined-only', str(args.build_dir / 'RsMatter.ino.elf')], text=True))
    print('Verified cache-off libc definitions: ' + json.dumps(cache_symbols, sort_keys=True))
    if fingerprint!=source_fingerprint(): raise ValueError('Source changed during compilation; rebuild required')
    if not args.skip_compile: stamp.write_text(fingerprint+'\n')
    segments = [(offset, (args.build_dir / filename).read_bytes()) for _, offset, filename in SEGMENTS]
    validate_segments(segments)
    if (b'LHBRIDGE-RUNTIME-V1-ESP32' not in segments[-1][1] or VERSION.encode() not in segments[-1][1]
            or b'Lighthouse-Bridge Matter Test' not in segments[-1][1]):
        raise ValueError('Application is not this Matter prototype')
    output = ROOT / 'dist/rs-matter'
    output.mkdir(parents=True, exist_ok=True)
    shutil.rmtree(ROOT/'dist/tasmota',ignore_errors=True)
    parts, checksums = [], []
    for (name, offset, _), (_, payload) in zip(SEGMENTS, segments):
        filename = f'lighthouse-rs-matter-{name}.bin'
        (output / filename).write_bytes(payload)
        digest = hashlib.sha256(payload).hexdigest()
        checksums.append(f'{digest}  {filename}\n')
        parts.append({'offset': offset, 'path': f'firmware/rs-matter/{filename}?sha={digest}'})
    ota_name = f'lighthouse-bridge-{VERSION}.lhota'
    ota = package(segments[-1][1])
    (output / ota_name).write_bytes(ota)
    checksums.append(f'{hashlib.sha256(ota).hexdigest()}  {ota_name}\n')
    (output / 'SHA256SUMS').write_text(''.join(checksums))
    (output / 'manifest.json').write_text(json.dumps({
        'name': 'Lighthouse Bridge — rs-matter prototype', 'version': VERSION,
        'new_install_prompt_erase': True, 'new_install_improv_wait_time': 0,
        'builds': [{'chipFamily': 'ESP32', 'parts': parts}],
    }, indent=2) + '\n')
    info = {'version': VERSION, 'engine':'rs-matter','rs_matter_revision':RS_REV,'rust_toolchain':'1.97.0.0','arduino_esp32': CORE, 'nimble': '2.5.2',
            'app_bytes': len(segments[-1][1]), 'app_slot_bytes': APP_LIMIT,
            'cache_safe_libc': cache_symbols, 'hardware_validated': False, 'stations': 16, 'bridge_name': 'Lighthouse-Bridge Matter Test',
            'device_type': 'on_off_light', 'dashboard': True, 'local_api': True, 'ota': True}
    (output / 'build-info.json').write_text(json.dumps(info, indent=2) + '\n')
    archive = output / f'lighthouse-bridge-{VERSION}.zip'
    temporary = archive.with_suffix('.zip.tmp')
    metadata=json.loads(subprocess.check_output(['cargo','+esp','metadata','--locked','--format-version','1','--filter-platform','xtensa-esp32-none-elf','--manifest-path',str(crate/'Cargo.toml')]))
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED,strict_timestamps=False) as bundle:
        for file in sorted(output.glob('*.bin')):
            bundle.write(file, file.name)
        bundle.write(output / ota_name, ota_name)
        for name in ['SHA256SUMS', 'build-info.json']:
            bundle.write(output / name, name)
        bundle.write(ROOT / 'experimental/rs-matter/README.md', 'READ-ME-FIRST.md')
        bundle.write(ROOT / 'VERSION.md', 'VERSION.md')
        bundle.write(ROOT / 'LICENSE', 'LICENSE')
        for path in ['experimental/rs-matter/RsMatter.ino', *[str(p.relative_to(ROOT)) for p in (ROOT/'experimental/rs-matter').glob('*.h')], 'experimental/rs-matter/rs_platform.cpp', 'experimental/rs-matter/rust/Cargo.toml', 'experimental/rs-matter/rust/Cargo.lock', *[str(p.relative_to(ROOT)) for p in (ROOT/'experimental/rs-matter/rust/src').glob('*.rs')],
                     *[str(p.relative_to(ROOT)) for p in (ROOT / 'experimental/matter/MatterBridge').glob('*.h')], 'partitions.csv',
                     *[str(p.relative_to(ROOT)) for p in (ROOT / 'include').glob('*.h')],
                     'src/main.cpp', 'tools/build_rs_matter.py', 'tools/build_matter.py', 'tools/matter_dashboard.py', 'tools/ota_package.py', 'tools/rs_matter_link.py', 'tools/requirements-matter-build.txt',
                     'experimental/rs-matter/README.md', 'VERSION.md', 'LICENSE']:
            bundle.write(ROOT / path, 'source/' + path)
        for file in (ROOT / 'third_party').iterdir():
            if file.is_file():
                bundle.write(file, 'third-party-licenses/' + file.name)
                bundle.write(file, 'source/third_party/' + file.name)
        for dependency in metadata['packages']:
            directory=Path(dependency['manifest_path']).parent
            candidates=set()
            for parent in [directory,*list(directory.parents)[:2]]:
                for pattern in ['LICENSE*','NOTICE*','COPYING*']:
                    candidates.update(file for file in parent.glob(pattern) if file.is_file())
                if candidates: break
            for file in sorted(candidates):
                bundle.write(file,Path('third-party-licenses/rust')/(dependency['name']+'-'+dependency['version'])/file.name)
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
