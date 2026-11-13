#pragma once
#include <ArduinoJson.h>
#include "bridge_core.h"
#include <algorithm>

namespace bridge {
struct StatusDiscovery {
  char name[48]{};
  char mac[18]{};
  uint8_t type = 1;
  int rssi = 0;
};
struct StatusSnapshot {
  StationConfig configs[kStations];
  StationState states[kStations];
  bool readQueued[kStations]{};
  StatusDiscovery found[kStations];
  unsigned foundCount = 0;
  uint32_t now = 0, scanFinishedMs = 0;
  bool scanning = false, foundTruncated = false, maintenance = false;
};
static_assert(sizeof(StatusSnapshot) <= 6144, "Keep the status snapshot bounded");

// The sink receives bounded blocks, never a whole response String. Keep the last
// block until the next write so an object's final brace can be removed safely,
// including when the serialized header ends exactly at a block boundary.
template<class Sink, size_t Capacity = 256> class StatusWriter {
  Sink& sink;
  uint8_t buffer[Capacity];
  size_t used = 0;
  bool good = true;
public:
  explicit StatusWriter(Sink& output) : sink(output) {}
  bool flush() {
    if (!good) return false;
    if (used && sink.write(buffer,used) != used) good = false;
    used = 0; return good;
  }
  size_t write(uint8_t value) { return write(&value,1); }
  size_t write(const uint8_t* bytes, size_t count) {
    size_t offset = 0;
    while (offset < count && good) {
      if (used == Capacity && !flush()) break;
      const size_t take = std::min(count-offset,Capacity-used);
      std::memcpy(buffer+used,bytes+offset,take); used += take; offset += take;
    }
    return offset;
  }
  size_t text(const char* value) { return write(reinterpret_cast<const uint8_t*>(value),std::strlen(value)); }
  bool dropLast(uint8_t expected) {
    if (!good || !used || buffer[used-1] != expected) return false;
    --used; return true;
  }
};
inline void writeStatusStation(JsonObject item, const StationConfig& config,
                               const StationState& state, bool queued, uint32_t now, bool matter) {
  item["id"] = config.id; item["name"] = config.name; item["mac"] = config.mac;
  item["address_type"] = config.addressType; item["alexa_compat"] = config.alexaCompat;
  item["configured"] = config.mac[0] != '\0';
  if (state.hasRequest) item["requested"] = targetName(state.requested);
  else item["requested"] = nullptr;
  item["pending"] = state.pending; item["busy"] = state.busy;
  item["reading"] = state.reading || queued; item["read_active"] = state.reading; item["read_queued"] = queued;
  item["reachable"] = freshMeasurement(state,now); item["read_failures"] = state.readFailures;
  item["ble_error"] = state.bleError; item["last_read_attempt_ms"] = state.lastReadAttemptMs;
  item["requested_ms"] = state.requestedMs; item["alexa_target_pending"] = !matter && hueTargetPending(state,now);
  item["command_sent"] = state.sent; item["command_confirmed"] = state.commandConfirmed;
  item["command_status"] = commandStatus(state,now); item["last_sent_ms"] = state.lastSentMs;
  item["observed"] = observedName(state.observed); item["observed_raw"] = state.observed;
  item["observed_ms"] = state.observedMs; item["error"] = state.error;
}
// This exact serializer is used by firmware and native tests. One snapshot owns
// all string lifetimes and the measurement/uptime pair; no lock is held for IO.
template<class Sink, size_t Capacity> bool streamStatus(const JsonDocument& header,
    const StatusSnapshot& view, StatusWriter<Sink,Capacity>& writer, bool matter) {
  if (header.overflowed()) return false;
  serializeJson(header,writer);
  if (!writer.dropLast('}')) return false;
  if (header.size()) writer.text(",");
  writer.text("\"stations\":[");
  StaticJsonDocument<JSON_OBJECT_SIZE(32)+128> item;
  bool first = true;
  for (unsigned i = 0; i < kStations; ++i) {
    if (!view.configs[i].id) continue;
    item.clear(); writeStatusStation(item.to<JsonObject>(),view.configs[i],view.states[i],view.readQueued[i],view.now,matter);
    if (item.overflowed()) return false;
    if (!first) writer.text(",");
    first = false;
    serializeJson(item,writer);
  }
  writer.text("],\"discovered\":[");
  if (view.foundCount > kStations) return false;
  for (unsigned i = 0; i < view.foundCount; ++i) {
    if (i) writer.text(",");
    item.clear(); const auto& found = view.found[i];
    item["name"] = found.name; item["mac"] = found.mac;
    item["address_type"] = found.type; item["rssi"] = found.rssi;
    serializeJson(item,writer);
  }
  writer.text("]}"); return writer.flush();
}
} // namespace bridge
