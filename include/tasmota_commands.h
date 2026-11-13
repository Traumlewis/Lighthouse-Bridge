#pragma once
#include "matter_config.h"
namespace bridge {
inline size_t matterLabelBytes(const char* name) {
  size_t length = std::strlen(name);
  if (length > 32) length = 32;
  while (length && (static_cast<uint8_t>(name[length]) & 0xc0) == 0x80) --length;
  return length;
}
struct MatterCommandAcceptance { unsigned station = 0; bool on = false; };
// Caller holds the shared state lock. The result never mutates observations.
inline MatterCommandAcceptance acceptTasmotaCommand(const StationConfig* configs, StationState* states,
    const MatterRegistry& registry, uint16_t endpoint, unsigned command, uint32_t now,
    bool ready, bool blocked) {
  MatterCommandAcceptance result;
  if (!ready || blocked || command > 2 || !endpoint || endpoint == 0xffff) return result;
  for (unsigned i = 0; i < kStations; ++i) {
    if (!configs[i].id || !configs[i].mac[0]) continue;
    const int binding = matterBindingIndex(registry,configs[i].hueId,configs[i].mac);
    if (binding < 0 || registry.entries[binding].endpoint != endpoint) continue;
    if (command == 2 && !freshMeasurement(states[i],now)) return result;
    result.on = command == 2 ? !measuredOn(states[i]) : command == 1;
    request(states[i],result.on ? Target::On : Target::Sleep,now);
    result.station = configs[i].id;
    break;
  }
  return result;
}
}
