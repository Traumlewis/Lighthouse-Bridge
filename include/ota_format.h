#pragma once
#include <cstdint>
#include <cstring>

namespace bridge {
constexpr unsigned kOtaHeaderSize = 64;
constexpr char kOtaMagic[] = "LHBRIDGE-ESP32-OTA1";
constexpr char kOtaProduct[] = "LHBRIDGE-RUNTIME-V1-ESP32";
inline bool parseOtaHeader(const uint8_t* header, uint32_t capacity, uint32_t& length, uint8_t* digest) {
  if (std::memcmp(header, kOtaMagic, sizeof(kOtaMagic)) != 0) return false;
  for (unsigned i = sizeof(kOtaMagic); i < 24; ++i) if (header[i]) return false;
  for (unsigned i = 60; i < 64; ++i) if (header[i]) return false;
  length = static_cast<uint32_t>(header[24]) | (static_cast<uint32_t>(header[25]) << 8) |
           (static_cast<uint32_t>(header[26]) << 16) | (static_cast<uint32_t>(header[27]) << 24);
  if (length < 288 || length > capacity) return false;
  std::memcpy(digest, header + 28, 32);
  return true;
}
#if defined(CONFIG_IDF_TARGET_ESP32C3)
constexpr uint16_t kOtaChipId = 5;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr uint16_t kOtaChipId = 9;
#else
constexpr uint16_t kOtaChipId = 0;
#endif
inline bool classicEsp32Image(const uint8_t* image) {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  // The XIAO S3 uses 8 MiB flash and OPI PSRAM. Reject the 4 MiB QSPI profile
  // on that board and the 8 MiB image on the SuperMini.
#if defined(ARDUINO_XIAO_ESP32S3)
  if (image[3] >> 4 != 3) return false;
#else
  if (image[3] >> 4 != 2) return false;
#endif
#endif
  return image[0] == 0xe9 && image[12] == (kOtaChipId & 255) && image[13] == (kOtaChipId >> 8) &&
         image[32] == 0x32 && image[33] == 0x54 && image[34] == 0xcd && image[35] == 0xab;
}
} // namespace bridge
