#!/usr/bin/env python3
"""Build Lighthouse Bridge for one of the supported ESP32 boards."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile
from ota_package import package
from firmware_assets import APP_LIMIT, ROOT, render_dashboard, validate_segments
from rs_boards import BOARDS
from rs_matter_link import validate_cache_safe_symbols, validate_diagnostics_symbols, validate_wifi_wrapper

VERSION = '0.1.31'
RS_REV = '43f61585a439f1a1e90924b696a6280850ef0145'
CORE = '3.3.12'
SEGMENTS = [('bootloader', 0x1000, 'RsMatter.ino.bootloader.bin'),
            ('partitions', 0x8000, 'RsMatter.ino.partitions.bin'),
            ('boot-app0', 0xe000, 'boot_app0.bin'),
            ('app', 0x10000, 'RsMatter.ino.bin')]



BUILD_SOURCES = (
    'tools/build_rs_matter.py', 'tools/firmware_assets.py', 'tools/ota_package.py',
    'tools/rs_matter_link.py', 'tools/rs_boards.py', 'tools/requirements-matter-build.txt',
)


def source_fingerprint():
    paths = [ROOT / name for name in ('src/main.cpp', 'src/dashboard.html', 'partitions.csv')]
    paths += [ROOT / name for name in BUILD_SOURCES]
    for directory, patterns in [('include', ['*.h']), ('firmware', ['*.h', '*.cpp', '*.ino']),
                                ('firmware/rust', ['Cargo.toml', 'Cargo.lock']), ('firmware/rust/src', ['*.rs'])]:
        for pattern in patterns:
            paths.extend((ROOT / directory).glob(pattern))
    digest=hashlib.sha256()
    for path in sorted(set(paths)):
        digest.update(str(path.relative_to(ROOT)).encode());digest.update(path.read_bytes())
    return digest.hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--board', choices=BOARDS, default='esp32')
    parser.add_argument('--arduino-cli', default='arduino-cli')
    parser.add_argument('--build-dir', type=Path, default=ROOT / '.pio/rs-matter-build')
    parser.add_argument('--skip-compile', action='store_true', help='Package a previously compiled matching sketch')
    args = parser.parse_args()
    board=BOARDS[args.board]
    fqbn = f"esp32:esp32:{board['arduino']}:PartitionScheme={board.get('partition', 'min_spiffs')}{board['options']}"
    segments_spec=[(name,board['boot'] if name=='bootloader' else offset,filename) for name,offset,filename in SEGMENTS]
    if args.board!='esp32' and args.build_dir==ROOT/'.pio/rs-matter-build':
        args.build_dir=ROOT/('.pio/rs-matter-build-'+args.board)
    library_info = json.loads(subprocess.check_output([args.arduino_cli, 'lib', 'list', '--format', 'json']))
    libraries = {entry['library']['name']: entry['library'] for entry in library_info['installed_libraries']}
    for name, version in [('NimBLE-Arduino', '2.5.2'), ('ArduinoJson', '6.21.5'), ('WiFiManager', '2.0.17')]:
        if libraries.get(name, {}).get('version') != version:
            raise SystemExit(f'Install {name}@{version} using Arduino CLI first')
    sketch = ROOT / ('.pio/rs-matter-sketch' if args.board=='esp32' else '.pio/rs-matter-sketch-'+args.board) / 'RsMatter'
    sketch.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / 'firmware/RsMatter.ino', sketch)
    shutil.copy2(ROOT / 'partitions.csv', sketch)
    (sketch / 'web_ui_gzip.h').write_text(render_dashboard(VERSION, sketch / 'matter-ui.html'))
    shutil.copy2(ROOT/'firmware/rs_platform.cpp',sketch/'rs_platform.cpp')
    crate=ROOT/'firmware/rust'
    rustlib=crate/'target'/board['target']/'release/liblighthouse_rs_matter.a'
    fingerprint=hashlib.sha256((source_fingerprint()+args.board).encode()).hexdigest()
    stamp=args.build_dir/'lighthouse-source-sha256'
    if args.skip_compile and (not stamp.exists() or stamp.read_text().strip()!=fingerprint):
        raise ValueError('Compiled rs-matter source fingerprint does not match; rebuild required')
    host_config='-include '+str(ROOT/'firmware/rs_nimble_config.h')+' '
    if not args.skip_compile:
        version=subprocess.check_output(['rustc','+esp','--version'],text=True)
        if not version.startswith('rustc 1.97.0'): raise ValueError('Espressif Rust 1.97.0.0 required')
        subprocess.run(['cargo','+esp','build','--locked','-Z','build-std=core,alloc','--target',board['target'],'--release','--manifest-path',str(crate/'Cargo.toml')],check=True)
        # The CLI selects the installed core; refuse silent upgrades/downgrades.
        platforms = json.loads(subprocess.check_output([args.arduino_cli, 'core', 'list', '--format', 'json']))['platforms']
        if not any(p['id'] == 'esp32:esp32' and p.get('installed_version') == CORE for p in platforms):
            raise SystemExit(f'Install esp32:esp32@{CORE} using Arduino CLI first')
        # Resolve SDK archives first, then Rust inside the same rescan group.
        # Whole-archive before the group overrides cache-safe libc with weak
        # compiler_builtins and crashes during flash initialization.
        subprocess.run([args.arduino_cli, 'compile', '--fqbn', fqbn, '--build-property',
                        f'upload.maximum_size={APP_LIMIT}', '--build-property',
                        'compiler.cpp.extra_flags='+host_config+'-DWM_NODEBUG -DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED -DCONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED -Oz -flto ' + ' '.join('-I' + str(path) for path in
                          [ROOT / 'include', ROOT / 'src', ROOT/'firmware', sketch]),
                        '--build-property', 'compiler.c.extra_flags='+host_config+'-DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_NIMBLE_MAX_CONNECTIONS=1 -DCONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED -DCONFIG_BT_NIMBLE_ROLE_BROADCASTER_DISABLED -Oz',
                        '--build-property', 'compiler.c.elf.extra_flags=-Oz -flto -flto-partition=one -Wl,--wrap=esp_wifi_init',
                        '--build-property', 'compiler.libraries.ldflags=-Wl,--undefined=memcpy,--undefined=memmove,--undefined=memset,--undefined=memcmp,--undefined=strlen -lc '+str(rustlib),
                        '--build-path', str(args.build_dir.resolve()), str(sketch)], check=True)
    properties = subprocess.check_output([args.arduino_cli, 'compile', '--fqbn', fqbn,
                                          '--show-properties=expanded', str(sketch)], text=True)
    properties_by_name = dict(line.split('=', 1) for line in properties.splitlines() if '=' in line)
    flash_mib = board.get('flash_mib', 4)
    if properties_by_name.get('build.flash_size') != f'{flash_mib}MB':
        raise ValueError('Arduino flash size does not match the board profile')
    if 'psram_type' in board and properties_by_name.get('build.psram_type') != board['psram_type']:
        raise ValueError('Arduino PSRAM mode does not match the board profile')
    compiler_path = next(line.split('=', 1)[1] for line in properties.splitlines()
                         if line.startswith('compiler.path='))
    nm = Path(compiler_path) / ('riscv32-esp-elf-nm' if board['chip']==5 else 'xtensa-esp-elf-nm')
    nm_output = subprocess.check_output(
        [str(nm), '--defined-only', str(args.build_dir / 'RsMatter.ino.elf')], text=True)
    cache_symbols = validate_cache_safe_symbols(nm_output, board['family'])
    diagnostic_symbols = validate_diagnostics_symbols(nm_output, board['family'])
    wifi_wrapper = validate_wifi_wrapper(nm_output)
    print('Verified cache-off libc definitions: ' + json.dumps(cache_symbols, sort_keys=True))
    if fingerprint!=hashlib.sha256((source_fingerprint()+args.board).encode()).hexdigest(): raise ValueError('Source changed during compilation; rebuild required')
    if not args.skip_compile: stamp.write_text(fingerprint+'\n')
    segments = [(offset, (args.build_dir / filename).read_bytes()) for _, offset, filename in segments_spec]
    validate_segments(segments, board['chip'], board['boot'], flash_mib)
    if (b'LHBRIDGE-RUNTIME-V1-ESP32' not in segments[-1][1] or VERSION.encode() not in segments[-1][1]
            or b'Lighthouse-Bridge' not in segments[-1][1]):
        raise ValueError('Application is not the expected Lighthouse Bridge build')
    output = ROOT / 'dist/rs-matter'
    if args.board!='esp32': output=output/args.board
    output.mkdir(parents=True, exist_ok=True)
    parts, checksums = [], []
    for (name, offset, _), (_, payload) in zip(segments_spec, segments):
        filename = f'lighthouse-rs-matter-{name}.bin'
        (output / filename).write_bytes(payload)
        digest = hashlib.sha256(payload).hexdigest()
        checksums.append(f'{digest}  {filename}\n')
        parts.append({'offset': offset, 'path': f'firmware/rs-matter/'+('' if args.board=='esp32' else args.board+'/')+f'{filename}?sha={digest}'})
    ota_name = f'lighthouse-bridge-{VERSION}.lhota'
    ota = package(segments[-1][1],board['chip'])
    (output / ota_name).write_bytes(ota)
    checksums.append(f'{hashlib.sha256(ota).hexdigest()}  {ota_name}\n')
    (output / 'SHA256SUMS').write_text(''.join(checksums))
    (output / 'manifest.json').write_text(json.dumps({
        'name': 'Lighthouse Bridge', 'version': VERSION,
        'new_install_prompt_erase': True, 'new_install_improv_wait_time': 0,
        'builds': [{'chipFamily': board['family'], 'parts': parts}],
    }, indent=2) + '\n')
    info = {'version': VERSION, 'board':args.board, 'chip_family':board['family'], 'engine':'rs-matter','rs_matter_revision':RS_REV,'rust_toolchain':'1.97.0.0','arduino_esp32': CORE, 'nimble': '2.5.2',
            'app_bytes': len(segments[-1][1]), 'app_slot_bytes': APP_LIMIT,
            'cache_safe_libc': cache_symbols, 'allocation_diagnostics': diagnostic_symbols, 'wifi_init_wrapper': wifi_wrapper, 'hardware_validated': False, 'stations': 16, 'bridge_name': 'Lighthouse-Bridge',
            'device_type': 'on_off_light', 'dashboard': True, 'local_api': True, 'ota': True}
    (output / 'build-info.json').write_text(json.dumps(info, indent=2) + '\n')
    archive = output / f'lighthouse-bridge-{VERSION}.zip'
    temporary = archive.with_suffix('.zip.tmp')
    metadata=json.loads(subprocess.check_output(['cargo','+esp','metadata','--locked','--format-version','1','--filter-platform',board['target'],'--manifest-path',str(crate/'Cargo.toml')]))
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED,strict_timestamps=False) as bundle:
        for file in sorted(output.glob('*.bin')):
            bundle.write(file, file.name)
        bundle.write(output / ota_name, ota_name)
        for name in ['SHA256SUMS', 'build-info.json']:
            bundle.write(output / name, name)
        bundle.write(ROOT / 'firmware/README.md', 'READ-ME-FIRST.md')
        bundle.write(ROOT / 'VERSION.md', 'VERSION.md')
        bundle.write(ROOT / 'LICENSE', 'LICENSE')
        for path in ['firmware/RsMatter.ino', *[str(p.relative_to(ROOT)) for p in (ROOT/'firmware').glob('*.h')], 'firmware/rs_platform.cpp', 'firmware/rust/Cargo.toml', 'firmware/rust/Cargo.lock', *[str(p.relative_to(ROOT)) for p in (ROOT/'firmware/rust/src').glob('*.rs')],
                     'partitions.csv',
                     *[str(p.relative_to(ROOT)) for p in (ROOT / 'include').glob('*.h')],
                     'src/main.cpp', 'src/dashboard.html', *BUILD_SOURCES,
                     'firmware/README.md', 'VERSION.md', 'LICENSE']:
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
            raise ValueError('Corrupt firmware ZIP')
    temporary.replace(archive)
    print(f'Lighthouse Bridge {VERSION}: {info["app_bytes"]}/{APP_LIMIT} bytes; unchanged slots/NVS; no hardware validation.')


if __name__ == '__main__':
    main()
