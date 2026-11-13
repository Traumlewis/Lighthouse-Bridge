#pragma once
#include <stddef.h>
#include <stdint.h>

namespace bridge {
// Transport.tryWrite returns positive progress, 0 for disconnect, -1 for a
// temporary shortage and -2 for a permanent error. No whole-body allocation.
template<class Transport> size_t writeSocketBody(Transport& transport, const uint8_t* data,
                                                size_t size, uint32_t started) {
  size_t sent = 0;
  uint32_t progress = transport.now();
  while (sent < size) {
    const uint32_t now = transport.now();
    if (now-started >= 8000 || now-progress >= 2500) break;
    const size_t count = size-sent < 256 ? size-sent : 256;
    const int written = transport.tryWrite(data+sent,count);
    if (written > 0 && size_t(written) <= count) {
      sent += size_t(written); progress = transport.now();
    } else if (written != -1) break;
    transport.pause();
  }
  return sent;
}
} // namespace bridge
