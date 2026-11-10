import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import re
import unittest
from urllib.parse import urlsplit, parse_qs

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / '_site'


class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []

    def handle_starttag(self, tag, attrs):
        self.links.extend(value for name, value in attrs if name in ('href', 'src', 'manifest'))


class PagesTests(unittest.TestCase):
    def test_embedded_dashboard_translations(self):
        text = (ROOT / 'include/web_ui.h').read_text()
        source = text.split('const T=', 1)[1].split(';\n', 1)[0]
        translations = json.loads(source)
        self.assertEqual(set(translations), {'en', 'de'})
        self.assertEqual(set(translations['en']), set(translations['de']))
        for key in re.findall(r'data-t="([^"]+)"', text):
            for language in translations:
                self.assertTrue(translations[language].get(key), (language, key))
        self.assertIn('<html lang="en">', text)

    def test_complete_english_and_german_page_translations(self):
        source = (SITE / 'translations.js').read_text().split('export const translations =', 1)[1].strip()
        translations = json.loads(source.removesuffix(';'))
        self.assertEqual(set(translations), {'en', 'de'})
        self.assertEqual(set(translations['en']), set(translations['de']))
        html = (SITE / 'index.html').read_text()
        self.assertIn('<html lang="en">', html)
        keys = re.findall(r'data-i18n(?:-content|-aria-label)?="([^"]+)"', html)
        for language, entries in translations.items():
            for key in keys + ['ready', 'unavailable', 'unavailable_version', 'build']:
                with self.subTest(language=language, key=key):
                    self.assertTrue(entries.get(key), 'Every displayed key needs a translation')

    def test_manifest_only_classic_esp32_with_nvs_preserving_segments(self):
        manifest = json.loads((SITE / 'manifest.json').read_text())
        self.assertEqual(len(manifest['builds']), 1)
        build = manifest['builds'][0]
        self.assertEqual(build['chipFamily'], 'ESP32')
        self.assertEqual([p['offset'] for p in build['parts']], [0x1000, 0x8000, 0xe000, 0x10000])
        self.assertEqual((SITE / urlsplit(build['parts'][0]['path']).path).read_bytes()[0], 0xe9)
        self.assertEqual((SITE / urlsplit(build['parts'][3]['path']).path).read_bytes()[0], 0xe9)
        for part in build['parts']:
            url = urlsplit(part['path'])
            self.assertEqual(parse_qs(url.query)['sha'][0], hashlib.sha256((SITE / url.path).read_bytes()).hexdigest())
        self.assertTrue(manifest['new_install_prompt_erase'])
        self.assertEqual(manifest['new_install_improv_wait_time'], 0)

    def test_web_update_preserves_existing_nvs_after_sector_erases(self):
        manifest = json.loads((SITE / 'manifest.json').read_text())
        memory = bytearray(b'\x55' * (4 * 1024 * 1024))
        saved_nvs = bytes(range(256)) * 80
        memory[0x9000:0xe000] = saved_nvs
        for part in manifest['builds'][0]['parts']:
            payload = (SITE / urlsplit(part['path']).path).read_bytes()
            offset = part['offset']
            sector_start = offset // 4096 * 4096
            sector_end = (offset + len(payload) + 4095) // 4096 * 4096
            self.assertTrue(sector_end <= 0x9000 or sector_start >= 0xe000)
            memory[sector_start:sector_end] = b'\xff' * (sector_end - sector_start)
            memory[offset:offset + len(payload)] = payload
        self.assertEqual(memory[0x9000:0xe000], saved_nvs)
        # Reproduces why the previous merged-at-zero update lost credentials.
        merged = (SITE / 'firmware/lighthouse-bridge-esp32-merged.bin').read_bytes()
        self.assertEqual(merged[0x9000:0xe000], b'\xff' * 0x5000)

    def test_all_page_assets_resolve_below_project_subpath(self):
        parser = Links()
        parser.feed((SITE / 'index.html').read_text())
        for link in parser.links:
            if link.startswith(('https:', '#')):
                continue
            with self.subTest(link=link):
                self.assertFalse(link.startswith('/'), 'Project Pages requires relative URLs')
                self.assertTrue((SITE / urlsplit(link).path).exists(), link)

    def test_all_lazy_loaded_vendor_modules_resolve(self):
        for file in (SITE / 'vendor').glob('*.js'):
            for relative in re.findall(r'["\x27](\./[^"\x27]+\.js)["\x27]', file.read_text()):
                self.assertTrue((file.parent / relative).is_file(), relative)

    def test_published_checksum_matches_image(self):
        info = json.loads((SITE / 'build-info.json').read_text())
        digest = hashlib.sha256((SITE / 'firmware/lighthouse-bridge-esp32-merged.bin').read_bytes()).hexdigest()
        self.assertEqual(info['sha256'], digest)
        self.assertEqual((SITE / 'firmware/SHA256SUMS').read_text().split()[0], digest)
        for line in (SITE / 'firmware/SHA256SUMS').read_text().splitlines():
            checksum, filename = line.split()
            self.assertEqual(hashlib.sha256((SITE / 'firmware' / filename).read_bytes()).hexdigest(), checksum)


if __name__ == '__main__':
    unittest.main()
