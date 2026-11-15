#include "ota_format.h"
#include <cassert>
#include <initializer_list>
int main() {
  uint8_t image[288]{};image[0]=0xe9;image[32]=0x32;image[33]=0x54;image[34]=0xcd;image[35]=0xab;
  for (unsigned chip: {0u,5u,9u}) {
    image[12]=chip;image[13]=0;
    assert(bridge::classicEsp32Image(image)==(chip==bridge::kOtaChipId));
  }
  image[12]=bridge::kOtaChipId;image[32]=0;
  assert(!bridge::classicEsp32Image(image));
}
