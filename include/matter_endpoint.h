#pragma once
#include <cstdint>
namespace bridge {
// SDK 1.5 invokes shutdown callbacks even for clusters whose init never ran.
// Delete never-initialized metadata first, using the public unlinking API.
template<class Sdk> bool discardMatterEndpoint(Sdk& sdk, typename Sdk::Handle endpoint, bool initialized) {
  if (!initialized) {
    auto cluster = sdk.firstCluster(endpoint);
    while (cluster) {
      auto next = sdk.nextCluster(cluster);
      if (!sdk.destroyCluster(cluster)) return false;
      cluster = next;
    }
  }
  return sdk.destroyEndpoint(endpoint);
}

// Learn the byte-heap cost of completed lights before attempting another one.
// This is a conservative admission policy, not a guarantee against SDK OOM.
struct MatterLightBudget {
  uint32_t cost = 4096;
  bool allows(uint32_t available) const { return available >= 2048 && available - 2048 >= cost; }
  void record(uint32_t before, uint32_t after) {
    if (before > after && before - after > cost) cost = before - after;
  }
};
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
