import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from rs_matter_link import CACHE_OFF_SYMBOLS, validate_cache_safe_symbols


class CacheOffLinkTests(unittest.TestCase):
    def listing(self, address=0x400914AC, kind='T'):
        return '\n'.join(f'{address:08x} {kind} {name}' for name in CACHE_OFF_SYMBOLS)

    def test_sdk_iram_and_rom_definitions(self):
        for address, kind in [(0x400914AC, 'T'), (0x4000C3C0, 'A')]:
            self.assertEqual(len(validate_cache_safe_symbols(self.listing(address, kind))), 5)

    def test_published_boot_crash_address_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Unsafe cache-off symbol: memmove'):
            validate_cache_safe_symbols(self.listing().replace('400914ac T memmove', '401649c0 W memmove'))

    def test_flash_strong_and_iram_weak_are_rejected(self):
        for address, kind in [(0x401649C0, 'T'), (0x400914AC, 'W'), (0x3FFB0000, 'D')]:
            with self.assertRaisesRegex(ValueError, 'Unsafe cache-off symbol'):
                validate_cache_safe_symbols(self.listing(address, kind))

    def test_missing_or_duplicate_definitions_fail_closed(self):
        with self.assertRaisesRegex(ValueError, 'Missing cache-off symbol: memmove'):
            validate_cache_safe_symbols('\n'.join(line for line in self.listing().splitlines() if not line.endswith(' memmove')))
        with self.assertRaisesRegex(ValueError, 'Duplicate cache-off symbol: memmove'):
            validate_cache_safe_symbols(self.listing() + '\n400914ac T memmove')
