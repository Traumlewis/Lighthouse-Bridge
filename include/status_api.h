#pragma once
#include <ArduinoJson.h>
#include "bridge_core.h"
#include <algorithm>
#include <cstdlib>
#include <new>

namespace bridge {
struct StatusDiscovery {
  char name[48]{};
  char mac[18]{};
  uint8_t type = 1;
  int rssi = 0;
};
struct StatusStation {
  StationConfig config;
  StationState state;
  bool readQueued = false;
};
struct StatusAllocator {
  void* (*allocate)(size_t) = std::malloc;
  void (*release)(void*) = std::free;
};
struct StatusSnapshot {
  StatusStation* stations[kStations]{};
  StatusDiscovery* found[kStations]{};
  unsigned foundCount = 0;
  uint32_t now = 0, scanFinishedMs = 0;
  bool scanning = false, foundTruncated = false, maintenance = false;
  bool valid = true;
  StatusAllocator allocator;
  explicit StatusSnapshot(StatusAllocator memory = {}) : allocator(memory) {}
  StatusSnapshot(const StatusSnapshot&) = delete;
  StatusSnapshot& operator=(const StatusSnapshot&) = delete;
  ~StatusSnapshot() { clear(); }
  void clear() {
    for (unsigned i = 0; i < kStations; ++i) {
      allocator.release(stations[i]); allocator.release(found[i]);
      stations[i] = nullptr; found[i] = nullptr;
    }
    foundCount = 0; valid = false;
  }
  template<class T> T* make() {
    void* storage = allocator.allocate(sizeof(T));
    return storage ? new(storage) T{} : nullptr;
  }
  // Caller holds the state lock. No IO or borrowed mutable strings.
  bool capture(const StationConfig* configs, const StationState* states,
               const bool* queued, const StatusDiscovery* discovered,
               unsigned count, uint32_t uptime) {
    clear();
    if (count > kStations) return false;
    now = uptime; foundCount = count;
    for (unsigned i = 0; i < kStations; ++i) {
      if (configs[i].id) {
        stations[i] = make<StatusStation>();
        if (!stations[i]) { clear(); return false; }
        stations[i]->config = configs[i]; stations[i]->state = states[i];
        stations[i]->readQueued = queued[i];
      }
      if (i < count) {
        found[i] = make<StatusDiscovery>();
        if (!found[i]) { clear(); return false; }
        *found[i] = discovered[i];
      }
    }
    valid = true; return true;
  }
};
static_assert(sizeof(StatusStation) <= 256, "Bound each data-heap allocation");
static_assert(sizeof(StatusDiscovery) <= 256, "Bound discovery allocation");
static_assert(sizeof(StatusSnapshot) <= 512, "Keep snapshot metadata on stack");
static_assert(sizeof(StatusSnapshot) + kStations * (sizeof(StatusStation) + sizeof(StatusDiscovery)) <= 6144,
              "Bound the complete snapshot, including full station/discovery capacity");

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
template<class Writer> bool streamStatus(const JsonDocument& header,
    const StatusSnapshot& view, Writer& writer, bool matter) {
  if (!view.valid || header.overflowed()) return false;
  serializeJson(header,writer);
  if (!writer.dropLast('}')) return false;
  if (header.size()) writer.text(",");
  writer.text("\"stations\":[");
  StaticJsonDocument<JSON_OBJECT_SIZE(32)+128> item;
  bool first = true;
  for (unsigned i = 0; i < kStations; ++i) {
    if (!view.stations[i]) continue;
    const auto& station = *view.stations[i];
    item.clear(); writeStatusStation(item.to<JsonObject>(),station.config,station.state,station.readQueued,view.now,matter);
    if (item.overflowed()) return false;
    if (!first) writer.text(",");
    first = false;
    serializeJson(item,writer);
  }
  writer.text("],\"discovered\":[");
  if (view.foundCount > kStations) return false;
  for (unsigned i = 0; i < view.foundCount; ++i) {
    if (i) writer.text(",");
    if (!view.found[i]) return false;
    item.clear(); const auto& found = *view.found[i];
    item["name"] = found.name; item["mac"] = found.mac;
    item["address_type"] = found.type; item["rssi"] = found.rssi;
    serializeJson(item,writer);
  }
  writer.text("]}"); return writer.flush();
}
} // namespace bridge
