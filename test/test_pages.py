import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / '_site'


class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []

    def handle_starttag(self, tag, attrs):
        self.links.extend(value for name, value in attrs if name in ('href', 'src', 'manifest'))


class PagesTests(unittest.TestCase):
    def test_manifest_only_classic_esp32_and_merged_offset_zero(self):
        manifest = json.loads((SITE / 'manifest.json').read_text())
        self.assertEqual(len(manifest['builds']), 1)
        build = manifest['builds'][0]
        self.assertEqual(build['chipFamily'], 'ESP32')
        self.assertEqual(build['parts'][0]['offset'], 0)
        image = SITE / build['parts'][0]['path']
        self.assertGreater(image.stat().st_size, 0x10000)
        self.assertEqual(image.read_bytes()[0x1000], 0xe9)
        self.assertEqual(image.read_bytes()[0x10000], 0xe9)
        self.assertTrue(manifest['new_install_prompt_erase'])
        self.assertEqual(manifest['new_install_improv_wait_time'], 0)

    def test_all_page_assets_resolve_below_project_subpath(self):
        parser = Links()
        parser.feed((SITE / 'index.html').read_text())
        for link in parser.links:
            if link.startswith(('https:', '#')):
                continue
            with self.subTest(link=link):
                self.assertFalse(link.startswith('/'), 'Project Pages requires relative URLs')
                self.assertTrue((SITE / link).exists(), link)

    def test_all_lazy_loaded_vendor_modules_resolve(self):
        for file in (SITE / 'vendor').glob('*.js'):
            for relative in re.findall(r'["\x27](\./[^"\x27]+\.js)["\x27]', file.read_text()):
                self.assertTrue((file.parent / relative).is_file(), relative)

    def test_published_checksum_matches_image(self):
        info = json.loads((SITE / 'build-info.json').read_text())
        digest = hashlib.sha256((SITE / 'firmware/lighthouse-bridge-esp32-merged.bin').read_bytes()).hexdigest()
        self.assertEqual(info['sha256'], digest)
        self.assertEqual((SITE / 'firmware/SHA256SUMS').read_text().split()[0], digest)


if __name__ == '__main__':
    unittest.main()
