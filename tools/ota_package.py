"""Branded update container for classic ESP32 Lighthouse Bridge app images."""
import hashlib
import struct

MAGIC = b'LHBRIDGE-ESP32-OTA1'
PRODUCT = b'LHBRIDGE-RUNTIME-V1-ESP32'
CAPACITY = 0x1e0000
HEADER_SIZE = 64


def validate_image(image, chip_id=0):
    if not 288 <= len(image) <= CAPACITY:
        raise ValueError('Application does not fit an OTA slot')
    if image[0] != 0xe9 or int.from_bytes(image[12:14], 'little') != chip_id or image[32:36] != b'\x32\x54\xcd\xab':
        raise ValueError('Application chip does not match target')
    if PRODUCT not in image:
        raise ValueError('Application is not Lighthouse Bridge')


def package(image, chip_id=0):
    validate_image(image, chip_id)
    return MAGIC.ljust(24, b'\0') + struct.pack('<I', len(image)) + hashlib.sha256(image).digest() + b'\0'*4 + image


def unpack(data, chip_id=0):
    if len(data) < HEADER_SIZE or data[:24] != MAGIC.ljust(24, b'\0') or data[60:64] != b'\0'*4:
        raise ValueError('Invalid update header')
    image = data[HEADER_SIZE:]
    if struct.unpack('<I', data[24:28])[0] != len(image) or hashlib.sha256(image).digest() != data[28:60]:
        raise ValueError('Invalid update size or digest')
    validate_image(image, chip_id)
    return image
