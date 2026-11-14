"""Reject classic-ESP32 builds whose cache-off libc calls resolve into flash."""
import re

CACHE_OFF_SYMBOLS = ('memcpy', 'memmove', 'memset', 'memcmp', 'strlen')


def validate_cache_safe_symbols(nm_output):
    """Check final definitions, not input archives or estimated linker sizes.

    ESP-IDF's strong ROM/IRAM implementations must win over Rust's weak
    compiler_builtins. Keep the SDK implementation (including its PSRAM
    workaround); do not replace it with hard-coded ROM addresses.
    """
    symbols = {}
    for line in nm_output.splitlines():
        match = re.fullmatch(r'\s*([0-9a-fA-F]+)\s+(\S)\s+(\S+)\s*', line)
        if match and match[3] in CACHE_OFF_SYMBOLS:
            if match[3] in symbols:
                raise ValueError(f'Duplicate cache-off symbol: {match[3]}')
            symbols[match[3]] = (int(match[1], 16), match[2])
    for name in CACHE_OFF_SYMBOLS:
        if name not in symbols:
            raise ValueError(f'Missing cache-off symbol: {name}')
        address, kind = symbols[name]
        # Classic ESP32 executable mask ROM or internal instruction RAM only.
        safe_address = (0x40000000 <= address < 0x40070000 or
                        0x40080000 <= address < 0x400A0000)
        if kind not in ('T', 'A') or not safe_address:
            raise ValueError(f'Unsafe cache-off symbol: {name} at {address:#x} ({kind})')
    return {name: f'{symbols[name][0]:#010x}' for name in CACHE_OFF_SYMBOLS}


def validate_diagnostics_symbols(nm_output):
    """Keep the heap failure hook and its data usable with the flash cache off.

    GCC LTO can discard an inline function's IRAM section attribute. Check
    the final definitions, including the local-symbol suffixes it generates.
    """
    prefixes = {
        'allocation_failed_hook': r'_ZN13rsDiagnosticsL?16allocationFailedE',
        'format': r'_ZN13rsDiagnosticsL13failureFormatE',
        'failures': r'_ZN13rsDiagnosticsL8failuresE',
        'last_report_ms': r'_ZN13rsDiagnosticsL12lastReportMsE',
    }
    found = {}
    for line in nm_output.splitlines():
        match = re.fullmatch(r'\s*([0-9a-fA-F]+)\s+(\S)\s+(\S+)\s*', line)
        if not match:
            continue
        for key, prefix in prefixes.items():
            if re.match(prefix, match[3]):
                if key in found:
                    raise ValueError(f'Duplicate allocation diagnostic symbol: {key}')
                address, kind = int(match[1], 16), match[2]
                safe = (0x40080000 <= address < 0x400A0000 and kind in ('t', 'T')
                        if key == 'allocation_failed_hook' else
                        0x3FF80000 <= address < 0x40000000 and kind in ('b', 'B', 'd', 'D', 'r', 'R'))
                if not safe:
                    raise ValueError(f'Unsafe allocation diagnostic symbol: {key} at {address:#x}')
                found[key] = f'{address:#010x}'
    for key in prefixes:
        if key not in found:
            raise ValueError(f'Missing allocation diagnostic symbol: {key}')
    return found
