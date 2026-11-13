#pragma once
#include <stddef.h>
#include <stdint.h>

namespace bridge {
// ESP32 flash is memory mapped. Bound socket writes rather than copying the
// compressed page to heap or asking lwIP to queue the complete page at once.
template<class Writer>
size_t writeHttpBody(Writer& writer, const uint8_t* data, size_t size) {
  size_t sent = 0;
  while (sent < size) {
    const size_t remaining = size - sent;
    const size_t count = remaining < 512 ? remaining : 512;
    const size_t written = writer.write(data + sent, count);
    if (!written || written > count) break;
    sent += written;
  }
  return sent;
}
} // namespace bridge
