import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from rs_matter_link import CACHE_OFF_SYMBOLS, validate_cache_safe_symbols, validate_diagnostics_symbols, validate_wifi_wrapper


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


class AllocationDiagnosticLinkTests(unittest.TestCase):
    def listing(self):
        return "\n".join([
            "40081338 T _ZN13rsDiagnosticsL16allocationFailedEjmPKc$lto_priv$0",
            "3ffc07c0 d _ZN13rsDiagnosticsL13failureFormatE",
            "3ffc5288 b _ZN13rsDiagnosticsL8failuresE$lto_priv$0",
            "3ffc5bc0 b _ZN13rsDiagnosticsL12lastReportMsE",
        ])

    def test_final_iram_hook_and_dram_data(self):
        self.assertEqual(len(validate_diagnostics_symbols(self.listing())), 4)

    def test_lto_inline_section_loss_and_missing_hook_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Unsafe allocation diagnostic symbol: allocation_failed_hook'):
            validate_diagnostics_symbols(self.listing().replace('40081338', '400db348'))
        with self.assertRaisesRegex(ValueError, 'Missing allocation diagnostic symbol: allocation_failed_hook'):
            validate_diagnostics_symbols('\n'.join(self.listing().splitlines()[1:]))

    def test_flash_data_and_duplicates_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Unsafe allocation diagnostic symbol: format'):
            validate_diagnostics_symbols(self.listing().replace('3ffc07c0', '3f401234'))
        with self.assertRaisesRegex(ValueError, 'Duplicate allocation diagnostic symbol: format'):
            validate_diagnostics_symbols(self.listing() + '\n' + self.listing().splitlines()[1])


class WifiWrapperLinkTests(unittest.TestCase):
    def test_linked_wrapper(self):
        self.assertEqual(validate_wifi_wrapper('400e1000 T __wrap_esp_wifi_init\n'), '0x400e1000')

    def test_missing_weak_or_duplicate_wrapper_is_rejected(self):
        for listing in ['', '400e1000 W __wrap_esp_wifi_init\n', '400e1000 T __wrap_esp_wifi_init\n400e1000 T __wrap_esp_wifi_init\n']:
            with self.assertRaisesRegex(ValueError, 'linked Wi-Fi init wrapper'):
                validate_wifi_wrapper(listing)
