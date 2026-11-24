"""Validation and dashboard packing shared by the rs-matter build and WebFlasher."""
import gzip
import hashlib
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
APP_LIMIT = 0x1e0000


def validate_segments(segments, chip_id=0, boot_offset=0x1000, flash_mib=4):
    expected = [boot_offset, 0x8000, 0xe000, 0x10000]
    if [offset for offset, _ in segments] != expected:
        raise ValueError('Unexpected firmware segment offsets')
    for index, (offset, data) in enumerate(segments):
        end = offset + (len(data) + 4095) // 4096 * 4096
        limit = expected[index + 1] if index < 3 else offset + APP_LIMIT
        if not data or end > limit or (offset < 0xe000 and end > 0x9000):
            raise ValueError('Segment exceeds its slot or erases NVS')
    for index in (0, 3):
        data = segments[index][1]
        if len(data) < 24 or data[0] != 0xe9 or struct.unpack_from('<H', data, 12)[0] != chip_id:
            raise ValueError('Firmware image does not match the board chip')
        if data[3] >> 4 != flash_mib.bit_length() - 1:
            raise ValueError('Firmware image does not match the board flash size')
    table = segments[1][1]
    entries = {}
    for offset in range(0, len(table) - 31, 32):
        if table[offset:offset + 2] != b'\xaaP':
            break
        _, kind, subtype, address, size, label, flags = struct.unpack_from('<HBBII16sI', table, offset)
        entries[label.rstrip(b'\0').decode()] = (kind, subtype, address, size, flags)
    if entries != {
        'nvs': (1, 2, 0x9000, 0x5000, 0), 'otadata': (1, 0, 0xe000, 0x2000, 0),
        'app0': (0, 16, 0x10000, APP_LIMIT, 0), 'app1': (0, 17, 0x1f0000, APP_LIMIT, 0),
        'spiffs': (1, 130, 0x3d0000, 0x30000, 0),
    }:
        raise ValueError('Firmware must keep the existing complete partition layout')


def render_dashboard(version, html_path):
    from zopfli.gzip import compress

    html = (ROOT / 'src/dashboard.html').read_text().replace('@FIRMWARE_VERSION@', version)
    html_path.write_text(html)
    compressed = compress(html.encode(), numiterations=15)
    if gzip.decompress(compressed) != html.encode():
        raise ValueError('Compressed dashboard round-trip failed')
    etag = hashlib.sha256(compressed).hexdigest()
    return (
        '#pragma once\nconst unsigned char WEB_UI_GZIP[] PROGMEM = {'
        + ','.join(str(byte) for byte in compressed)
        + '};\n'
        + f'const char WEB_UI_ETAG[] = "\\"{etag}\\"";\n'
    )
