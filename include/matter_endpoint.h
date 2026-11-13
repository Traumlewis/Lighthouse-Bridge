#pragma once
#include <cstdint>
namespace bridge {
// Call on the CHIP task after stack startup restored the SDK's allocation
// counter. The counter is read-only metadata; only SDK create() advances it.
// Resume is valid only below that counter. Never change an assigned identity.
template<class Sdk> typename Sdk::Handle stableMatterEndpoint(Sdk& sdk, uint16_t wanted,
    uint8_t flags, void* privateData) {
  if (!wanted || wanted == 0xffff || !sdk.next || sdk.next > 0xffff || sdk.find(wanted)) return nullptr;
  while (sdk.next <= wanted) {
    auto created = sdk.create(flags,privateData);
    if (!created) return nullptr;
    const uint16_t actual = sdk.id(created);
    if (actual != sdk.next) { sdk.destroy(created); return nullptr; }
    ++sdk.next;
    if (actual == wanted) return created;
    // Gaps are retired/reserved identities, not extra visible devices.
    if (!sdk.destroy(created)) return nullptr;
    sdk.yield();
  }
  return sdk.resume(wanted,flags,privateData);
}
} // namespace bridge
