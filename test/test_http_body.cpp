#include "http_body.h"
#include <assert.h>
#include <algorithm>
#include <vector>
#include <stdio.h>
struct Sink {
  std::vector<uint8_t> bytes;
  size_t limit = 512, failAt = 100000, calls = 0;
  size_t write(const uint8_t* data, size_t size) {
    assert(size <= 512);
    ++calls;
    if (bytes.size() >= failAt) return 0;
    size = std::min(size, std::min(limit, failAt - bytes.size()));
    bytes.insert(bytes.end(), data, data + size);
    return size;
  }
};
int main() {
  // Binary payload exceeds the owner's largest reported heap block; embedded
  // zeros, a short final chunk and partial socket writes must all survive.
  std::vector<uint8_t> body(16883);
  for (size_t i = 0; i < body.size(); ++i) body[i] = uint8_t(i);
  Sink normal;
  assert(bridge::writeHttpBody(normal, body.data(), body.size()) == body.size());
  assert(normal.bytes == body);
  Sink partial; partial.limit = 73;
  assert(bridge::writeHttpBody(partial, body.data(), body.size()) == body.size());
  assert(partial.bytes == body);
  Sink failed; failed.failAt = 777;
  assert(bridge::writeHttpBody(failed, body.data(), body.size()) == 777);
  assert(std::equal(failed.bytes.begin(), failed.bytes.end(), body.begin()));
  Sink empty;
  assert(bridge::writeHttpBody(empty, nullptr, 0) == 0 && empty.calls == 0);
  puts("Bounded binary HTTP writes: full, partial, disconnect and empty passed.");
}
