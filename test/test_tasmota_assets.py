import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import zipfile

ROOT=Path(__file__).resolve().parents[1]

class TasmotaAssetsTests(unittest.TestCase):
    def test_exact_layout_and_ota_payload(self):
        folder=ROOT/'dist/tasmota'
        manifest=json.loads((folder/'manifest.json').read_text())
        self.assertEqual(manifest['version'],'0.1.6-tasmota')
        build=manifest['builds'][0]
        self.assertEqual(build['chipFamily'],'ESP32')
        self.assertEqual([p['offset'] for p in build['parts']],[0x1000,0x8000,0xe000,0x10000])
        for part in build['parts']:
            path,digest=part['path'].split('?sha=')
            self.assertTrue(path.startswith('firmware/tasmota/lighthouse-tasmota-'))
            self.assertEqual(hashlib.sha256((folder/Path(path).name).read_bytes()).hexdigest(),digest)
        self.assertEqual((folder/'lighthouse-tasmota-partitions.bin').read_bytes(),
                         (ROOT/'dist/lighthouse-bridge-partitions.bin').read_bytes())
        app=(folder/'lighthouse-tasmota-app.bin').read_bytes()
        ota=(folder/'lighthouse-bridge-0.1.6-tasmota.lhota').read_bytes()
        self.assertLessEqual(len(app),0x1e0000)
        self.assertEqual(struct.unpack_from('<H',app,12)[0],0)
        self.assertEqual(ota[64:],app)
        self.assertEqual(struct.unpack_from('<I',ota,24)[0],len(app))
        self.assertEqual(ota[28:60],hashlib.sha256(app).digest())

    def test_source_distribution_is_complete_and_private_files_excluded(self):
        archive=ROOT/'dist/tasmota/lighthouse-bridge-0.1.6-tasmota.zip'
        with zipfile.ZipFile(archive) as bundle:
            names=set(bundle.namelist())
            self.assertFalse(any(set(Path(name).parts)&{'.codex-attachments','.cache','.git','.pio'} for name in names))
            for name in ['LICENSE-TASMOTA.txt','source/LICENSE','source/tools/build_tasmota.py',
                         'source/tasmota-upstream/.lighthouse-upstream','source/tasmota-upstream/lib/libesp32/berry/src/be_vm.c',
                         'source/tasmota-upstream/lib/libesp32/berry_matter/src/embedded/Matter_zzz_Lighthouse.be',
                         'source/tasmota-upstream/lib/libesp32/berry_matter/src/solidify/solidified_Matter_zzz_Lighthouse.h',
                         'source/tasmota-upstream/tasmota/lighthouse.cpp','source/experimental/tasmota/reference-partitions.bin']:
                self.assertIn(name,names)
            self.assertIn('GNU GENERAL PUBLIC LICENSE',bundle.read('LICENSE-TASMOTA.txt').decode())
            for header in (ROOT/'include').glob('*.h'):
                self.assertIn('source/'+str(header.relative_to(ROOT)),names)

    def test_extracted_source_preparation(self):
        with tempfile.TemporaryDirectory(prefix='lighthouse-source-') as temporary:
            folder=Path(temporary)
            with zipfile.ZipFile(ROOT/'dist/tasmota/lighthouse-bridge-0.1.6-tasmota.zip') as bundle:
                for name in bundle.namelist():
                    if name.startswith('source/'): bundle.extract(name,folder)
            source=folder/'source'
            result=subprocess.run([str(ROOT/'.tools/tasmota-venv/bin/python'),str(source/'tools/build_tasmota.py'),
                '--prepared','--source-tree',str(source/'tasmota-upstream'),'--prepare-only'],
                cwd=source,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90)
            self.assertEqual(result.returncode,0,result.stdout)
            self.assertTrue((source/'tasmota-upstream/tasmota/web_ui_gzip.h').is_file())

    def test_linked_matter_modules(self):
        import sys
        sys.path.insert(0,str(ROOT/'tools'))
        from build_tasmota import validate_runtime_modules
        validate_runtime_modules(ROOT/'.pio/tasmota-src/.pio/build/lighthouse/firmware.elf',
            ROOT/'.pio/tasmota-pio-core/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp-elf-nm')

    def test_staged_safety_boundaries(self):
        stage=ROOT/'.pio/tasmota-src'
        support=(stage/'tasmota/tasmota_support/support_esp32.ino').read_text()
        self.assertIn('void SettingsErase(uint8_t type) {\n  return;',support)
        self.assertIn('void EspPrepRestartToSafeBoot(void) {\n  return;',support)
        self.assertIn('bool EspPrepSwitchPartition(uint32_t state) {\n  return false;',support)
        init=(stage/'tasmota/tasmota_xdrv_driver/xdrv_52_7_berry_embedded.ino').read_text()
        self.assertNotIn('p.resize_fs_to_max()',init)
        self.assertNotIn('global.matter_device = matter.Device()',init)
        self.assertNotIn('do import tapp end',init)
        fs=(stage/'tasmota/tasmota_xdrv_driver/xdrv_50_filesystem.ino').read_text()
        init=fs[fs.index('void UfsInitOnce('):fs.index('// Called from tasmota.ino at restart.')]
        self.assertIn('lh_tasmota_mount_fs()',init)
        self.assertNotIn('begin(true',init)
        self.assertNotIn('FFat.begin',init)
        defines=(stage/'tasmota/tasmota_defines_for_berry.h').read_text()
        self.assertNotRegex(defines,r'(?m)^#define USE_WEBSERVER\b')
        self.assertNotRegex(defines,r'(?m)^#define USE_BLE_ESP32\b')
        wifi=(stage/'tasmota/tasmota_support/support_wifi.ino').read_text()
        self.assertIn('void WiFiSetSleepMode(void)\n{\n  return;',wifi)
        mqtt=(stage/'tasmota/tasmota_xdrv_driver/xdrv_02_9_mqtt.ino').read_text()
        self.assertIn('void MqttReconnect(void) {\n  return;',mqtt)
        features=(stage/'tasmota/lighthouse_features.h').read_text()
        for macro in ['USE_WEBSERVER','USE_BLE_ESP32','USE_EMULATION','USE_ARDUINO_OTA']:
            self.assertIn('#undef '+macro,features)
