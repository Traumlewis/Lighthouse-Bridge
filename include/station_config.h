#pragma once
#include "bridge_core.h"
#include <ArduinoJson.h>
#include <string>

namespace bridge {
inline int stationIndex(const StationConfig* stations, uint32_t id) {
  for (unsigned i = 0; i < kStations; ++i) if (id && stations[i].id == id) return i;
  return -1;
}

inline bool parseStationList(JsonVariantConst input, StationConfig* out,
                             const StationConfig* current, uint32_t& nextId,
                             bool restoring, bool legacy, std::string& error) {
  if (!input.is<JsonArrayConst>() || input.size() > kStations) { error = "CONFIG_LIMIT"; return false; }
  for (unsigned i = 0; i < kStations; ++i) out[i] = StationConfig{};
  if (legacy && nextId < 6) nextId = 6; // Retain all old device identities, including retired group 5.
  if (!nextId) nextId = 1;
  unsigned count = 0, position = 0;
  for (JsonVariantConst item : input.as<JsonArrayConst>()) {
    ++position;
    if (!item["name"].is<const char*>() || !validName(item["name"].as<const char*>())) {
      error = "CONFIG_NAME"; return false;
    }
    if (!item["mac"].is<const char*>()) { error = "CONFIG_MAC"; return false; }
    const char* mac = item["mac"];
    if (legacy && !*mac) continue; // Old empty slots are not dynamic entries.
    if (!legacy && item.containsKey("id") && !item["id"].is<uint32_t>()) { error = "CONFIG_IDS"; return false; }
    uint32_t id = legacy ? position : (item["id"].is<uint32_t>() ? item["id"].as<uint32_t>() : 0);
    if (!id && !restoring) {
      if (nextId == kRetiredGroupId) ++nextId;
      if (!nextId || nextId > kMaxStationId) { error = "CONFIG_IDS"; return false; }
      id = nextId++;
    } else if (!id || id == kRetiredGroupId || id > kMaxStationId ||
               (!restoring && stationIndex(current, id) < 0)) {
      error = "CONFIG_IDS"; return false;
    }
    if (restoring && nextId <= id) nextId = id + 1;
    auto& value = out[count];
    value.id = id;
    std::strcpy(value.name, item["name"].as<const char*>());
    if (*mac && !normalizeMac(mac, value.mac)) { error = "CONFIG_MAC"; return false; }
    value.addressType = 1;
    for (unsigned j = 0; j < count; ++j) {
      if (out[j].id == id) { error = "CONFIG_IDS"; return false; }
      if (*mac && !std::strcmp(out[j].mac, value.mac)) { error = "CONFIG_MAC"; return false; }
      if (!std::strcmp(out[j].name, value.name)) { error = "CONFIG_NAME"; return false; }
    }
    ++count;
  }
  return true;
}
// Caller holds the state lock and rejects active commands. A read still in flight
// belongs to its old array index: invalidate that generation even after reordering.
inline void reconcileStationStates(const StationConfig* next, const StationConfig* current,
                                   const StationState* states, StationState* out) {
  for (unsigned i = 0; i < kStations; ++i) {
    out[i] = StationState{};
    const int old = stationIndex(current, next[i].id);
    if (old >= 0 && !std::strcmp(current[old].mac, next[i].mac)) out[i] = states[old];
    out[i].generation = states[i].generation + 1;
    out[i].reading = false;
  }
}
inline void writeStationList(JsonArray array, const StationConfig* stations) {
  for (unsigned i = 0; i < kStations; ++i) {
    if (!stations[i].id) continue;
    auto item = array.createNestedObject();
    item["id"] = stations[i].id;
    item["name"] = stations[i].name;
    item["mac"] = stations[i].mac;
  }
}
} // namespace bridge
