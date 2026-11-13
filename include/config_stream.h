#pragma once
#include "status_api.h"
#include "json_stream.h"
#include "matter_config.h"

namespace bridge {
constexpr size_t kConfigRequestCapacity = JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(kStations) +
                                         kStations * JSON_OBJECT_SIZE(6);
struct CountSink {
  size_t size = 0;
  size_t write(const uint8_t*,size_t count) { size += count; return count; }
};
template<class Writer> bool streamConfiguration(Writer& writer, const StationConfig* configs,
                                               uint32_t nextId, bool response = false) {
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> header;
  if (response) { header["saved"] = true; header["restarting"] = true; }
  else header["next_id"] = nextId;
  serializeJson(header,writer);
  if (!writer.dropLast('}')) return false;
  writer.text(",\"stations\":["); bool first = true;
  StaticJsonDocument<JSON_OBJECT_SIZE(5)> item;
  for (unsigned i = 0; i < kStations; ++i) {
    if (!configs[i].id) continue;
    item.clear(); writeStationConfig(item.to<JsonObject>(),configs[i],!response);
    if (!first) writer.text(",");
    first = false;
    serializeJson(item,writer);
  }
  writer.text("]}"); return writer.flush();
}
template<class Writer> bool streamRegistry(Writer& writer, const MatterRegistry& registry) {
  StaticJsonDocument<JSON_OBJECT_SIZE(2)> header;
  header["version"] = 1; header["next_endpoint"] = registry.nextEndpoint;
  serializeJson(header,writer);
  if (!writer.dropLast('}')) return false;
  writer.text(",\"bindings\":[");
  StaticJsonDocument<JSON_OBJECT_SIZE(3)> item;
  for (unsigned i = 0; i < registry.count; ++i) {
    if (i) writer.text(",");
    item.clear(); const auto& entry = registry.entries[i];
    item["hue_id"] = entry.hueId; item["mac"] = entry.mac; item["endpoint"] = entry.endpoint;
    serializeJson(item,writer);
  }
  writer.text("]}"); return writer.flush();
}
} // namespace bridge
