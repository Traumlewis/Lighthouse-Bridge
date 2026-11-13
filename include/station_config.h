#pragma once
#include "bridge_core.h"
#include <ArduinoJson.h>
#include <string>
#include <algorithm>

namespace bridge {
// Bounded JSON storage for all 16 assignments, including legacy address_type,
// copied names/MACs and interned field names. Independent of request body length.
constexpr size_t kStationJsonCapacity = JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(kStations) +
  kStations * (JSON_OBJECT_SIZE(6) + 41 + 18) + 256;

inline int stationIndex(const StationConfig* stations, uint32_t id) {
  for (unsigned i = 0; i < kStations; ++i) if (id && stations[i].id == id) return i;
  return -1;
}
inline int stationHueIndex(const StationConfig* stations, uint32_t hueId) {
  for (unsigned i = 0; i < kStations; ++i) if (hueId && stations[i].id && stations[i].hueId == hueId) return i;
  return -1;
}
inline int stationIdForHue(const StationConfig* stations, uint32_t hueId) {
  const int index = stationHueIndex(stations, hueId);
  return index < 0 ? 0 : static_cast<int>(stations[index].id);
}
inline bool parseStationList(JsonVariantConst input, StationConfig* out,
                             const StationConfig* current, uint32_t& nextId,
                             bool restoring, bool legacy, std::string& error) {
  if (!input.is<JsonArrayConst>() || input.size() > kStations) { error = "CONFIG_LIMIT"; return false; }
  for (unsigned i = 0; i < kStations; ++i) out[i] = StationConfig{};
  if (legacy && nextId < 6) nextId = 6;
  if (!nextId) nextId = 1;
  // Reserve all retained slots before allocating new entries, regardless of input order.
  bool used[kStations + 1]{};
  unsigned sourcePosition = 0;
  for (JsonVariantConst item : input.as<JsonArrayConst>()) {
    ++sourcePosition;
    if (legacy && item["mac"].is<const char*>() && !*item["mac"].as<const char*>()) continue;
    if (!legacy && item.containsKey("id") && !item["id"].is<uint32_t>()) { error = "CONFIG_IDS"; return false; }
    const uint32_t id = legacy ? sourcePosition : item["id"].as<uint32_t>();
    const bool retain = restoring ? (legacy || item.containsKey("hue_id")) : id != 0;
    if (!retain) continue;
    if (!id || id > kStations || used[id] || (!restoring && stationIndex(current,id) < 0)) { error = "CONFIG_IDS"; return false; }
    used[id] = true;
  }
  uint32_t seenIds[kStations]{};
  unsigned count = 0, position = 0;
  for (JsonVariantConst item : input.as<JsonArrayConst>()) {
    ++position;
    if (!item["name"].is<const char*>() || !validName(item["name"].as<const char*>())) { error = "CONFIG_NAME"; return false; }
    if (!item["mac"].is<const char*>()) { error = "CONFIG_MAC"; return false; }
    const char* mac = item["mac"];
    if (legacy && !*mac) continue;
    if (!legacy && item.containsKey("id") && !item["id"].is<uint32_t>()) { error = "CONFIG_IDS"; return false; }
    const uint32_t inputId = legacy ? position : (item["id"].is<uint32_t>() ? item["id"].as<uint32_t>() : 0);
    uint32_t hueId = 0;
    if (restoring) {
      if (!inputId) { error = "CONFIG_IDS"; return false; }
      // Pre-0.3.6 records used the Hue identity directly as the local station ID.
      if (item.containsKey("hue_id")) {
        if (!item["hue_id"].is<uint32_t>()) { error = "CONFIG_IDS"; return false; }
        hueId = item["hue_id"].as<uint32_t>();
      } else hueId = inputId;
    } else if (inputId) {
      const int existing = stationIndex(current, inputId);
      if (existing < 0) { error = "CONFIG_IDS"; return false; }
      hueId = current[existing].hueId; // Client fields cannot replace Alexa identity.
    } else {
      if (nextId == kRetiredGroupId) ++nextId;
      if (!nextId || nextId > kMaxStationId) { error = "CONFIG_IDS"; return false; }
      hueId = nextId++;
    }
    if (!hueId || hueId == kRetiredGroupId || hueId > kMaxStationId) { error = "CONFIG_IDS"; return false; }
    if (restoring && nextId <= hueId) nextId = hueId + 1;
    auto& value = out[count];
    const bool retain = restoring ? (legacy || item.containsKey("hue_id")) : inputId != 0;
    uint32_t localId = inputId;
    if (!retain) { for (localId = 1; localId <= kStations && used[localId]; ++localId) {} }
    if (!localId || localId > kStations) { error = "CONFIG_LIMIT"; return false; }
    used[localId] = true;
    value.id = localId;
    value.hueId = hueId;
    std::strcpy(value.name, item["name"].as<const char*>());
    if (*mac && !normalizeMac(mac, value.mac)) { error = "CONFIG_MAC"; return false; }
    value.addressType = 1;
    if (item.containsKey("alexa_compat")) {
      if (!item["alexa_compat"].is<bool>()) { error = "CONFIG_PROFILE"; return false; }
      value.alexaCompat = item["alexa_compat"].as<bool>();
    } else if (!restoring && inputId) value.alexaCompat = current[stationIndex(current,inputId)].alexaCompat;
    for (unsigned j = 0; j < count; ++j) {
      if ((inputId && seenIds[j] == inputId) || out[j].hueId == hueId) { error = "CONFIG_IDS"; return false; }
      if (*mac && !std::strcmp(out[j].mac, value.mac)) { error = "CONFIG_MAC"; return false; }
      if (!std::strcmp(out[j].name, value.name)) { error = "CONFIG_NAME"; return false; }
    }
    seenIds[count++] = inputId;
  }
  std::sort(out,out+count,[](const StationConfig& a,const StationConfig& b){return a.id<b.id;});
  return true;
}
// History follows stable Alexa identity + MAC; invalidate reads at the old index.
inline void reconcileStationStates(const StationConfig* next, const StationConfig* current,
                                   const StationState* states, StationState* out) {
  for (unsigned i = 0; i < kStations; ++i) {
    out[i] = StationState{};
    const int old = stationHueIndex(current, next[i].hueId);
    if (old >= 0 && !std::strcmp(current[old].mac, next[i].mac)) out[i] = states[old];
    out[i].generation = states[i].generation + 1;
    out[i].reading = false;
  }
}
inline void writeStationList(JsonArray array, const StationConfig* stations, bool storage = false) {
  for (unsigned i = 0; i < kStations; ++i) {
    if (!stations[i].id) continue;
    auto item = array.createNestedObject();
    item["id"] = stations[i].id;
    if (storage) item["hue_id"] = stations[i].hueId;
    item["name"] = stations[i].name;
    item["mac"] = stations[i].mac;
    item["alexa_compat"] = stations[i].alexaCompat;
  }
}
} // namespace bridge
