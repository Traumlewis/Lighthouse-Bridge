#include "ota_format.h"
#include <cassert>
#include <initializer_list>
int main() {
  uint8_t image[288]{};image[0]=0xe9;image[32]=0x32;image[33]=0x54;image[34]=0xcd;image[35]=0xab;
#if defined(ARDUINO_XIAO_ESP32S3)
  image[3] = 0x3f;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  image[3] = 0x2f;
#endif
  for (unsigned chip: {0u,5u,9u}) {
    image[12]=chip;image[13]=0;
    assert(bridge::classicEsp32Image(image)==(chip==bridge::kOtaChipId));
  }
  image[12]=bridge::kOtaChipId;
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  assert(bridge::classicEsp32Image(image));
  image[3] ^= 0x10;
  assert(!bridge::classicEsp32Image(image));
  image[3] ^= 0x10;
#endif
  image[32]=0;
  assert(!bridge::classicEsp32Image(image));
}
