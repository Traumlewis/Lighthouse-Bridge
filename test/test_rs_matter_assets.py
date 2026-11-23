import hashlib
import json
from pathlib import Path
import struct
import unittest
import zipfile
from urllib.parse import urlsplit,parse_qs
ROOT=Path(__file__).resolve().parents[1]
class RsMatterAssetsTests(unittest.TestCase):
    def test_exact_layout_and_ota_payload(self):
        folder=ROOT/'dist/rs-matter'
        manifest=json.loads((folder/'manifest.json').read_text())
        self.assertEqual(manifest['version'],'0.1.29')
        build=manifest['builds'][0]
        self.assertEqual(build['chipFamily'],'ESP32')
        self.assertEqual([p['offset'] for p in build['parts']],[0x1000,0x8000,0xe000,0x10000])
        for part in build['parts']:
            path=urlsplit(part['path'])
            self.assertTrue(path.path.startswith('firmware/rs-matter/lighthouse-rs-matter-'))
            self.assertEqual(hashlib.sha256((folder/Path(path.path).name).read_bytes()).hexdigest(),parse_qs(path.query)['sha'][0])
        self.assertEqual((folder/'lighthouse-rs-matter-partitions.bin').read_bytes(),(ROOT/'_site/firmware/rs-matter/lighthouse-rs-matter-partitions.bin').read_bytes())
        app=(folder/'lighthouse-rs-matter-app.bin').read_bytes()
        for marker in [b'rs-matter protocol: module=',b'case_sessions=',b'commissioning_complete_persisted',b'rs-matter path: operation=',b'rs-matter endpoint: id=',b'loop_stack_bytes=']:
            self.assertIn(marker,app)
        ota=(folder/'lighthouse-bridge-0.1.29.lhota').read_bytes()
        self.assertLessEqual(len(app),0x1e0000)
        self.assertEqual(struct.unpack_from('<H',app,12)[0],0)
        self.assertEqual(ota[64:],app)
        self.assertEqual(struct.unpack_from('<I',ota,24)[0],len(app))
        self.assertEqual(ota[28:60],hashlib.sha256(app).digest())
        info=json.loads((folder/'build-info.json').read_text())
        self.assertEqual(info['engine'],'rs-matter')
        self.assertEqual(info['bridge_name'],'Lighthouse-Bridge')
        self.assertNotIn(b'Lighthouse-Bridge Matter Test',app)
        self.assertFalse(info['hardware_validated'])
        self.assertEqual(set(info['cache_safe_libc']), {'memcpy','memmove','memset','memcmp','strlen'})
        for address in info['cache_safe_libc'].values():
            self.assertTrue(0x40000000 <= int(address,16) < 0x40070000 or 0x40080000 <= int(address,16) < 0x400a0000)
    def test_complete_rebuild_sources_and_no_private_logs(self):
        with zipfile.ZipFile(ROOT/'dist/rs-matter/lighthouse-bridge-0.1.29.zip') as archive:
            names=set(archive.namelist())
            self.assertIn('source/firmware/rust/src/diagnostics.rs',names)
            self.assertIn('source/firmware/rust/src/case_read_test.rs',names)
            self.assertIn('source/firmware/rust/src/tests.rs',names)
            for name in ['firmware/RsMatter.ino','firmware/rs_platform.cpp','firmware/rs_diagnostics.h','firmware/rs_network.h','firmware/rust/Cargo.toml','firmware/rust/Cargo.lock','firmware/rust/src/lib.rs','firmware/rust/src/model.rs','firmware/rust/src/platform.rs','include/local_wifi_page.h','include/rs_wifi_portal.h','include/web_account.h','tools/rs_matter_link.py','tools/rs_boards.py','include/device_maintenance.h','firmware/rs_maintenance.h','include/gpio_buttons.h','include/remote_buttons.h','firmware/rs_remote.h','firmware/rs_station_info.h','src/main.cpp','src/dashboard.html','tools/firmware_assets.py','partitions.csv']:
                self.assertIn('source/'+name,names)
            self.assertFalse(any(any(x in name for x in ['.codex-attachments','/target/','tasmota-upstream']) for name in names))
    def test_removed_backend_not_published(self):
        self.assertFalse((ROOT/'_site/rs-matter.html').exists())
        self.assertFalse((ROOT/'_site/flasher-redirect.js').exists())
        self.assertFalse((ROOT/'_site/tasmota.html').exists())
        self.assertFalse((ROOT/'_site/tasmota-manifest.json').exists())
        self.assertFalse((ROOT/'_site/matter.html').exists())
        self.assertFalse((ROOT/'_site/matter-manifest.json').exists())
        self.assertFalse((ROOT/'_site/firmware/matter').exists())
        self.assertFalse((ROOT/'_site/firmware/lighthouse-bridge-esp32-merged.bin').exists())
