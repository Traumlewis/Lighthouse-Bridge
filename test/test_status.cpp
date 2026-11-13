#include "status_api.h"
#include "matter_config.h"
#include <cassert>
#include <iostream>
#include <string>
using namespace bridge;
struct Sink {
  char data[49152]{};
  size_t used = 0, calls = 0, maxChunk = 0;
  bool fail = false;
  size_t write(const uint8_t* bytes,size_t count) {
    ++calls; maxChunk = std::max(maxChunk,count);
    if (fail || used + count >= sizeof(data)) return 0;
    std::memcpy(data+used,bytes,count); used += count; return count;
  }
};
template<size_t Chunk> void check() {
  StatusSnapshot snapshot;
  snapshot.now = 90000; snapshot.foundCount = kStations;
  for (unsigned i = 0; i < kStations; ++i) {
    auto& cfg = snapshot.configs[i]; cfg.id = i+1; cfg.hueId = i+6;
    std::strcpy(cfg.name,"Station \"quote\" \\ path \xe2\x82\xac");
    std::strcpy(cfg.mac,"AA:BB:CC:DD:EE:01");
    auto& state = snapshot.states[i]; state.observed = i%2 ? 0 : 11;
    state.observedMs = 89000; state.reachable = true;
    state.hasRequest = true; state.pending = true; state.requested = Target::On; state.requestedMs = 89900;
    snapshot.readQueued[i] = i%2;
    std::memset(state.error,'X',sizeof(state.error)-1);
    std::strcpy(snapshot.found[i].name,"LHB-\"device\" \\ \xe2\x82\xac");
    std::strcpy(snapshot.found[i].mac,cfg.mac); snapshot.found[i].rssi = -60;
  }
  // This is much smaller than the old 32768-byte allocation, regardless of count.
  static_assert(sizeof(StatusSnapshot) < 6144,"bounded response snapshot");
  StaticJsonDocument<1024> header;
  header["version"] = "test"; header["uptime_ms"] = snapshot.now;
  Sink sink; StatusWriter<Sink,Chunk> writer(sink);
  assert(streamStatus(header,snapshot,writer,true));
  assert(sink.maxChunk <= Chunk);
  DynamicJsonDocument parsed(65536);
  assert(!deserializeJson(parsed,sink.data,sink.used));
  assert(parsed["stations"].size() == 16 && parsed["discovered"].size() == 16);
  assert(parsed["uptime_ms"] == 90000);
  for (unsigned i = 0; i < kStations; ++i) {
    const auto item = parsed["stations"][i];
    assert(item.size() == 26); // Preserve the existing station status contract.
    assert(item["id"] == i+1 && item["observed_ms"] == 89000);
    assert(item["observed_raw"] == (i%2 ? 0 : 11));
    assert(item["alexa_target_pending"] == false);
    assert(item["requested"] == "on" && item["pending"] == true);
    assert(item["reading"].as<bool>() == static_cast<bool>(i%2));
    assert(item["read_active"] == false && item["reachable"] == true);
    assert(std::strcmp(item["name"],snapshot.configs[i].name) == 0);
    assert(std::strlen(item["error"]) == 95);
    assert(std::strcmp(parsed["discovered"][i]["name"],snapshot.found[i].name) == 0);
  }
  // Exactly one closing brace is removed, including chunk-size boundaries and {}.
  StaticJsonDocument<32> empty;
  empty.to<JsonObject>(); StatusSnapshot none;
  Sink tiny; StatusWriter<Sink,Chunk> tinyWriter(tiny);
  assert(streamStatus(empty,none,tinyWriter,true));
  assert(std::string(tiny.data,tiny.used) == "{\"stations\":[],\"discovered\":[]}");
  Sink failed; failed.fail = true; StatusWriter<Sink,Chunk> failedWriter(failed);
  assert(!streamStatus(header,snapshot,failedWriter,true));
  assert(failed.calls == 1); // Never continue writing after a failed block.
}
int main() {
  check<1>(); check<2>(); check<7>(); check<16>(); check<256>(); check<4096>();
  StatusSnapshot view; view.now = 500;
  view.configs[0].id = 1; view.states[0].observed = 0; view.states[0].reachable = true;
  view.states[0].observedMs = 0xfffffff0U;
  StaticJsonDocument<1024> item;
  writeStatusStation(item.to<JsonObject>(),view.configs[0],view.states[0],true,view.now,true);
  assert(item["reachable"] == true && item["read_queued"] == true);
  view.states[0].reading = true;
  item.clear(); writeStatusStation(item.to<JsonObject>(),view.configs[0],view.states[0],false,view.now,true);
  assert(item["reading"] == true && item["read_active"] == true && item["read_queued"] == false);
  view.now = 60000;
  item.clear(); writeStatusStation(item.to<JsonObject>(),view.configs[0],view.states[0],false,view.now,true);
  assert(item["reachable"] == false);
  StationConfig configs[kStations];
  for (unsigned i = 0; i < kStations; ++i) {
    configs[i].id = i+1; configs[i].hueId = i+6;
    std::memset(configs[i].name,'a'+i,40);
    std::snprintf(configs[i].mac,sizeof(configs[i].mac),"AA:BB:CC:DD:EE:%02X",i);
  }
  DynamicJsonDocument config(kStationJsonCapacity);
  config["next_id"] = 30; writeStationList(config.createNestedArray("stations"),configs,true);
  assert(!config.overflowed());
  std::string saved; serializeJson(config,saved);
  DynamicJsonDocument restored(kStationJsonCapacity);
  assert(!deserializeJson(restored,saved));
  StationConfig parsed[kStations]; std::string error; uint32_t next = 30;
  assert(parseStationList(restored["stations"],parsed,nullptr,next,true,false,error));
  assert(std::strlen(parsed[15].name) == 40 && parsed[15].id == 16);
  MatterRegistry registry;
  registry.count = 32; registry.nextEndpoint = 100;
  for (unsigned i = 0; i < registry.count; ++i) {
    registry.entries[i].hueId = i+6; registry.entries[i].endpoint = i+3;
    std::snprintf(registry.entries[i].mac,sizeof(registry.entries[i].mac),"AA:BB:CC:DD:EE:%02X",i);
  }
  DynamicJsonDocument mapping(kMatterRegistryJsonCapacity);
  writeMatterRegistry(mapping.to<JsonObject>(),registry);
  assert(!mapping.overflowed());
  saved.clear(); serializeJson(mapping,saved);
  DynamicJsonDocument mapRestored(kMatterRegistryJsonCapacity);
  assert(!deserializeJson(mapRestored,saved));
  MatterRegistry restoredRegistry;
  assert(parseMatterRegistry(mapRestored.as<JsonVariantConst>(),restoredRegistry,error));
  assert(restoredRegistry.count == 32 && restoredRegistry.nextEndpoint == 100);
  std::cout << "Streamed status passed: 16 stations/results, bounded chunks/snapshot, escaped text, measurements, wrap/staleness and disconnects.\n";
}
