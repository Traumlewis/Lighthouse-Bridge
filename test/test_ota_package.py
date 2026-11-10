import sys
from pathlib import Path
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from ota_package import package, unpack, PRODUCT, CAPACITY

class OtaTests(unittest.TestCase):
 def image(self):
  data=bytearray(1000);data[0]=0xe9;data[32:36]=b'\x32\x54\xcd\xab';data[400:400+len(PRODUCT)]=PRODUCT
  return bytes(data)
 def test_roundtrip(self):
  image=self.image();self.assertEqual(unpack(package(image)),image)
 def test_corruption_truncation_trailing(self):
  encoded=package(self.image())
  for bad in [encoded[:-1],encoded+b'0',b'0'+encoded[1:],encoded[:700]+b'1'+encoded[701:],encoded[:63]+b'1'+encoded[64:]]:
   with self.assertRaises(ValueError):unpack(bad)
 def test_reject_wrong_chip_product_merged_and_oversize(self):
  image=self.image()
  for bad in [image[:12]+b'\x05'+image[13:],image.replace(PRODUCT,b'X'*len(PRODUCT)),b'\xff'*0x10000+image,image+bytes(CAPACITY)]:
   with self.assertRaises(ValueError):package(bad)
 def test_published_payload_when_available(self):
  root=Path(__file__).resolve().parents[1];path=root/'_site/firmware/lighthouse-bridge-v0.3.0.lhota'
  if not path.exists():self.skipTest('Generated Pages not built yet')
  self.assertEqual(unpack(path.read_bytes()),(root/'_site/firmware/lighthouse-bridge-app.bin').read_bytes())
