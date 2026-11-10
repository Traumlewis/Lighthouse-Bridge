#pragma once
#include "ota_format.h"
#include <algorithm>
#include <cstddef>

namespace bridge {
// IO supplies inactive-slot writes and incremental SHA-256. finish() validates
// the stream only; the HTTP handler must authorize the full request before commit.
template <typename IO> class OtaReceiver {
 public:
  explicit OtaReceiver(IO& io) : io(io) {}
  bool started = false, ready = false;
  void abort() { if (started) io.abort(); started = ready = false; }
  void reset() { abort(); prefixCount = expected = received = match = 0; product = false; }
  const char* feed(uint8_t* bytes, size_t count, uint32_t capacity) {
    if (ready) return reject("OTA_INVALID");
    if (prefixCount < sizeof(prefix)) {
      const size_t take = std::min(count, sizeof(prefix) - prefixCount);
      std::memcpy(prefix + prefixCount, bytes, take);
      prefixCount += take; bytes += take; count -= take;
      if (prefixCount < sizeof(prefix)) return nullptr;
      if (!parseOtaHeader(prefix, capacity, expected, digest) || !classicEsp32Image(prefix + kOtaHeaderSize))
        return reject("OTA_INVALID");
      if (!io.begin(expected)) return reject("OTA_WRITE_FAILED");
      started = true;
      const char* error = feed(prefix + kOtaHeaderSize, 288, capacity);
      if (error) return error;
    }
    if (!count) return nullptr;
    if (!started || count > expected - received) return reject("OTA_INVALID");
    for (size_t i = 0; i < count; ++i) {
      if (bytes[i] == static_cast<uint8_t>(kOtaProduct[match])) {
        if (++match == sizeof(kOtaProduct) - 1) { product = true; match = 0; }
      } else match = bytes[i] == static_cast<uint8_t>(kOtaProduct[0]) ? 1 : 0;
    }
    if (!io.write(bytes, count)) return reject("OTA_WRITE_FAILED");
    received += count;
    return nullptr;
  }
  const char* finish() {
    if (!started || ready || received != expected || !product) return reject("OTA_INVALID");
    uint8_t actual[32];
    if (!io.finishHash(actual) || std::memcmp(actual, digest, 32)) return reject("OTA_INVALID");
    ready = true;
    return nullptr;
  }
  // Called only after the firmware writer successfully selects the next boot slot.
  void committed() { started = ready = false; }
 private:
  IO& io;
  uint8_t prefix[kOtaHeaderSize + 288]{}, digest[32]{};
  size_t prefixCount = 0;
  uint32_t expected = 0, received = 0;
  unsigned match = 0;
  bool product = false;
  const char* reject(const char* error) { abort(); return error; }
};
} // namespace bridge
