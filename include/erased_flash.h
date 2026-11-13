#pragma once
#include <stddef.h>
#include <stdint.h>
namespace bridge {
// Read the entire proposed filesystem before granting permission to format it.
// A failed read or any existing byte fails closed; no sampling of just a header.
template<class Reader> bool completelyErasedFlash(Reader read,size_t length) {
  if (!length) return false;
  uint8_t buffer[256];
  for (size_t offset = 0; offset < length;) {
    const size_t size = length-offset < sizeof(buffer) ? length-offset : sizeof(buffer);
    if (!read(offset,buffer,size)) return false;
    for (size_t i = 0; i < size; ++i) if (buffer[i] != 0xff) return false;
    offset += size;
  }
  return true;
}
}
