import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest
from urllib.parse import urlsplit, parse_qs

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from build_matter import validate_segments, APP_LIMIT


class MatterLayoutTests(unittest.TestCase):
    def parts(self):
        manifest = json.loads((ROOT / '_site/matter-manifest.json').read_text())
        self.assertEqual(manifest['version'], '0.1.0-matter')
        self.assertEqual([b['chipFamily'] for b in manifest['builds']], ['ESP32'])
        return [(p['offset'], (ROOT / '_site' / urlsplit(p['path']).path).read_bytes())
                for p in manifest['builds'][0]['parts']]

    def test_actual_images_fit_and_keep_both_slots(self):
        parts = self.parts()
        validate_segments(parts)
        self.assertLessEqual(len(parts[-1][1]), APP_LIMIT)
        self.assertEqual(parts[1][1], (ROOT / 'dist/lighthouse-bridge-partitions.bin').read_bytes())

    def test_reject_oversize_nonclassic_and_changed_partitions(self):
        parts = self.parts()
        invalid = copy.deepcopy(parts)
        invalid[-1] = (0x10000, invalid[-1][1].ljust(APP_LIMIT + 1, b'\xff'))
        with self.assertRaises(ValueError): validate_segments(invalid)
        invalid = copy.deepcopy(parts)
        data = bytearray(invalid[-1][1]); struct.pack_into('<H', data, 12, 9)
        invalid[-1] = (0x10000, bytes(data))
        with self.assertRaises(ValueError): validate_segments(invalid)
        invalid = copy.deepcopy(parts)
        data = bytearray(invalid[1][1]); struct.pack_into('<I', data, 4, 0xa000)
        invalid[1] = (0x8000, bytes(data))
        with self.assertRaises(ValueError): validate_segments(invalid)
        invalid = copy.deepcopy(parts); invalid[0] = (0, invalid[0][1])
        with self.assertRaises(ValueError): validate_segments(invalid)

    def test_split_flash_and_stable_return_preserve_nvs(self):
        memory = bytearray(b'\xff' * 0x400000)
        saved = bytes(range(256)) * 80
        memory[0x9000:0xe000] = saved
        stable = json.loads((ROOT / '_site/manifest.json').read_text())['builds'][0]['parts']
        stable_parts = [(p['offset'], (ROOT / '_site' / urlsplit(p['path']).path).read_bytes()) for p in stable]
        for sequence in [self.parts(), stable_parts]:
            for address, data in sequence:
                end = (address + len(data) + 4095) // 4096 * 4096
                self.assertTrue(end <= 0x9000 or address >= 0xe000)
                memory[address:end] = b'\xff' * (end - address)
                memory[address:address + len(data)] = data
            self.assertEqual(memory[0x9000:0xe000], saved)

    def test_manifest_hashes_and_translated_page(self):
        manifest = json.loads((ROOT / '_site/matter-manifest.json').read_text())
        for part in manifest['builds'][0]['parts']:
            url = urlsplit(part['path']); data = (ROOT / '_site' / url.path).read_bytes()
            self.assertEqual(parse_qs(url.query)['sha'][0], hashlib.sha256(data).hexdigest())
        import re
        translations = json.loads((ROOT / 'site/translations.js').read_text().split('export const translations =',1)[1].strip().removesuffix(';'))
        html = (ROOT / 'site/matter.html').read_text()
        for key in re.findall(r'data-i18n(?:-aria-label)?="([^"]+)"', html):
            for language in ['en','de']:
                self.assertTrue(translations[language][key], (language,key))


if __name__ == '__main__':
    unittest.main()
