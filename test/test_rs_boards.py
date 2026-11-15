"""Board profiles must preserve NVS/slots and reject cross-chip images."""
import sys
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from rs_boards import BOARDS
from build_matter import validate_segments
from rs_matter_link import validate_cache_safe_symbols

class BoardTests(unittest.TestCase):
 def test_profiles_and_complete_layout(self):
  self.assertEqual(set(BOARDS),{'esp32','weact-c3','c3-supermini','s3-supermini'})
  folder=ROOT/'dist'
  table=(folder/'lighthouse-bridge-partitions.bin').read_bytes()
  for name,board in BOARDS.items():
   image=bytearray(288);image[0]=0xe9;image[12:14]=board['chip'].to_bytes(2,'little')
   segments=[(board['boot'],bytes(image)),(0x8000,table),(0xe000,b'\xff'*8192),(0x10000,bytes(image))]
   validate_segments(segments,board['chip'],board['boot'])
   for other in (0,5,9):
    if other!=board['chip']:
     with self.assertRaises(ValueError):validate_segments(segments,other,board['boot'])
   wrong=segments.copy();wrong[0]=(0x1000 if board['boot']==0 else 0,bytes(image))
   with self.assertRaises(ValueError):validate_segments(wrong,board['chip'],board['boot'])
   wrong=segments.copy();wrong[1]=(0x9000,table)
   with self.assertRaises(ValueError):validate_segments(wrong,board['chip'],board['boot'])
   wrong=segments.copy();wrong[-1]=(0x10000,bytes(image)+b'\0'*0x1e0000)
   with self.assertRaises(ValueError):validate_segments(wrong,board['chip'],board['boot'])
 def test_chip_specific_cache_off_symbols(self):
  symbols=('memcpy','memmove','memset','memcmp','strlen')
  for family,iram in [('ESP32',0x40081000),('ESP32-C3',0x40380000),('ESP32-S3',0x40378000)]:
   good='\n'.join(f'{iram+i*16:08x} T {s}'for i,s in enumerate(symbols))
   self.assertEqual(len(validate_cache_safe_symbols(good,family)),5)
   for bad in (0x42001000,0x400D1000,0x3FC81000):
    with self.assertRaises(ValueError):validate_cache_safe_symbols(good.replace(f'{iram:08x}',f'{bad:08x}'),family)
