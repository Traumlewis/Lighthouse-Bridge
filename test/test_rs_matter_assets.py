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
        self.assertEqual(manifest['version'],'0.1.0-rs-matter')
        build=manifest['builds'][0]
        self.assertEqual(build['chipFamily'],'ESP32')
        self.assertEqual([p['offset'] for p in build['parts']],[0x1000,0x8000,0xe000,0x10000])
        for part in build['parts']:
            path=urlsplit(part['path'])
            self.assertTrue(path.path.startswith('firmware/rs-matter/lighthouse-rs-matter-'))
            self.assertEqual(hashlib.sha256((folder/Path(path.path).name).read_bytes()).hexdigest(),parse_qs(path.query)['sha'][0])
        self.assertEqual((folder/'lighthouse-rs-matter-partitions.bin').read_bytes(),(ROOT/'dist/lighthouse-bridge-partitions.bin').read_bytes())
        app=(folder/'lighthouse-rs-matter-app.bin').read_bytes()
        ota=(folder/'lighthouse-bridge-0.1.0-rs-matter.lhota').read_bytes()
        self.assertLessEqual(len(app),0x1e0000)
        self.assertEqual(struct.unpack_from('<H',app,12)[0],0)
        self.assertEqual(ota[64:],app)
        self.assertEqual(struct.unpack_from('<I',ota,24)[0],len(app))
        self.assertEqual(ota[28:60],hashlib.sha256(app).digest())
        info=json.loads((folder/'build-info.json').read_text())
        self.assertEqual(info['engine'],'rs-matter')
        self.assertFalse(info['hardware_validated'])
    def test_complete_rebuild_sources_and_no_private_logs(self):
        with zipfile.ZipFile(ROOT/'dist/rs-matter/lighthouse-bridge-0.1.0-rs-matter.zip') as archive:
            names=set(archive.namelist())
            for name in ['experimental/rs-matter/RsMatter.ino','experimental/rs-matter/rs_platform.cpp','experimental/rs-matter/rust/Cargo.toml','experimental/rs-matter/rust/Cargo.lock','experimental/rs-matter/rust/src/lib.rs','experimental/rs-matter/rust/src/model.rs','experimental/rs-matter/rust/src/platform.rs','tools/matter_dashboard.py','src/main.cpp','partitions.csv']:
                self.assertIn('source/'+name,names)
            self.assertFalse(any(any(x in name for x in ['.codex-attachments','/target/','tasmota-upstream']) for name in names))
    def test_removed_backend_not_published(self):
        self.assertFalse((ROOT/'_site/tasmota.html').exists())
        self.assertFalse((ROOT/'_site/tasmota-manifest.json').exists())
        self.assertTrue((ROOT/'_site/rs-matter.html').exists())
