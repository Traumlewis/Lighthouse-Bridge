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
