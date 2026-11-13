#pragma once
#include "station_config.h"

namespace bridge {
constexpr uint16_t kMatterAggregatorEndpoint = 2;
constexpr uint32_t kMatterEndpointLimit = 0xfffe;
constexpr unsigned kMatterBindingCapacity = kStations * 2;
constexpr size_t kMatterRegistryJsonCapacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(kMatterBindingCapacity) +
  kMatterBindingCapacity * (JSON_OBJECT_SIZE(3) + 18) + 256;
struct MatterBinding {
  uint32_t hueId = 0;
  char mac[18]{};
  uint16_t endpoint = 0;
};
struct MatterRegistry {
  uint32_t nextEndpoint = 3;
  unsigned count = 0;
  MatterBinding entries[kMatterBindingCapacity];
};
inline int matterBindingIndex(const MatterRegistry& registry, uint32_t hueId, const char* mac) {
  for (unsigned i = 0; i < registry.count; ++i)
    if (registry.entries[i].hueId == hueId && !std::strcmp(registry.entries[i].mac, mac)) return i;
  return -1;
}
inline bool sameMatterStation(const StationConfig& a, const StationConfig& b) {
  return a.id && b.id && a.mac[0] && b.mac[0] && a.hueId == b.hueId && !std::strcmp(a.mac,b.mac);
}
inline bool appendMatterBinding(MatterRegistry& out, const StationConfig& config, uint16_t endpoint,
                               std::string& error) {
  if (out.count >= kMatterBindingCapacity || !endpoint || endpoint == kMatterAggregatorEndpoint) {
    error = "MATTER_IDS_EXHAUSTED"; return false;
  }
  MatterBinding& entry = out.entries[out.count++];
  entry.hueId = config.hueId; entry.endpoint = endpoint;
  std::strncpy(entry.mac, config.mac, sizeof(entry.mac)-1);
  return true;
}
inline bool allocateMatterBinding(MatterRegistry& out, const StationConfig& config, std::string& error) {
  if (out.nextEndpoint > kMatterEndpointLimit) { error = "MATTER_IDS_EXHAUSTED"; return false; }
  return appendMatterBinding(out,config,static_cast<uint16_t>(out.nextEndpoint++),error);
}
inline bool seedMatterRegistry(const StationConfig* current, MatterRegistry& out, std::string& error) {
  out = MatterRegistry{};
  // The original single-station prototype used child 1 and aggregator 2.
  bool first = true;
  for (unsigned i = 0; i < kStations; ++i) {
    if (!current[i].id || !current[i].mac[0]) continue;
    if (first) { if (!appendMatterBinding(out,current[i],1,error)) return false; first = false; }
    else if (!allocateMatterBinding(out,current[i],error)) return false;
  }
  return true;
}
// Keep current + proposed bindings until the station record is committed. A
// failed station save can waste IDs, but cannot renumber a surviving device.
inline bool planMatterRegistry(const StationConfig* current, const StationConfig* next,
                               const MatterRegistry& saved, MatterRegistry& out, std::string& error) {
  out = MatterRegistry{}; out.nextEndpoint = saved.nextEndpoint;
  for (unsigned i = 0; i < kStations; ++i) {
    if (!current[i].id || !current[i].mac[0]) continue;
    const int existing = matterBindingIndex(saved,current[i].hueId,current[i].mac);
    if (existing >= 0) {
      if (!appendMatterBinding(out,current[i],saved.entries[existing].endpoint,error)) return false;
    } else if (!allocateMatterBinding(out,current[i],error)) return false;
  }
  for (unsigned i = 0; i < kStations; ++i) {
    if (!next[i].id || !next[i].mac[0]) continue;
    if (matterBindingIndex(out,next[i].hueId,next[i].mac) >= 0) continue;
    // Never revive a retired/candidate endpoint merely because it remains in
    // the checkpoint from an interrupted save. Only current survivors retain it.
    if (!allocateMatterBinding(out,next[i],error)) return false;
  }
  return true;
}
inline void writeMatterRegistry(JsonObject root, const MatterRegistry& registry) {
  root["version"] = 1; root["next_endpoint"] = registry.nextEndpoint;
  auto entries = root.createNestedArray("bindings");
  for (unsigned i = 0; i < registry.count; ++i) {
    const auto& entry = registry.entries[i]; auto object = entries.createNestedObject();
    object["hue_id"] = entry.hueId; object["mac"] = entry.mac; object["endpoint"] = entry.endpoint;
  }
}
inline bool parseMatterRegistry(JsonVariantConst root, MatterRegistry& out, std::string& error) {
  out = MatterRegistry{};
  if (!root.is<JsonObjectConst>() || !root["version"].is<unsigned>() || root["version"] != 1 || !root["next_endpoint"].is<uint32_t>() ||
      root["next_endpoint"].as<uint32_t>() < 3 || root["next_endpoint"].as<uint32_t>() > kMatterEndpointLimit+1 ||
      !root["bindings"].is<JsonArrayConst>() || root["bindings"].size() > kMatterBindingCapacity) {
    error = "MATTER_REGISTRY_INVALID"; return false;
  }
  out.nextEndpoint = root["next_endpoint"];
  for (auto object : root["bindings"].as<JsonArrayConst>()) {
    if (!object["hue_id"].is<uint32_t>() || !object["endpoint"].is<uint32_t>() || !object["mac"].is<const char*>()) {
      error = "MATTER_REGISTRY_INVALID"; return false;
    }
    const uint32_t hueId = object["hue_id"], endpoint = object["endpoint"];
    const char* mac = object["mac"];
    char normalized[18]{};
    if (!hueId || hueId == kRetiredGroupId || hueId > kMaxStationId || !normalizeMac(mac,normalized) ||
        !normalized[0] || std::strcmp(mac,normalized) || !endpoint || endpoint == kMatterAggregatorEndpoint ||
        endpoint >= out.nextEndpoint || endpoint > kMatterEndpointLimit) {
      error = "MATTER_REGISTRY_INVALID"; return false;
    }
    for (unsigned i = 0; i < out.count; ++i) {
      if (out.entries[i].endpoint == endpoint ||
          (out.entries[i].hueId == hueId && !std::strcmp(out.entries[i].mac,mac))) {
        error = "MATTER_REGISTRY_INVALID"; return false;
      }
    }
    auto& entry = out.entries[out.count++];
    entry.hueId = hueId; entry.endpoint = endpoint; std::strcpy(entry.mac,mac);
  }
  return true;
}
} // namespace bridge
