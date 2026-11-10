#include "hue_api.h"
#include <cassert>
#include <iostream>
#include <regex>
#include <set>

using namespace bridge;
static StationState states[kStations];
static bool configured[kStations] = {true, true, true, true};
static uint32_t ids[kStations] = {};
static unsigned queued = 0;
static int lastSlot = -2;
static Target lastTarget = Target::Sleep;

static bool enqueue(int slot, Target target) {
  bool accepted = false;
  for (unsigned i = 0; i < kStations; ++i) {
    if (configured[i] && (slot < 0 || slot == static_cast<int>(ids[i]))) {
      request(states[i], target);
      accepted = true;
    }
  }
  if (accepted) { ++queued; lastSlot = slot; lastTarget = target; }
  return accepted;
}

static std::string uri(uint32_t mac, unsigned slot) {
  return "/api/test-user/lights/" + std::to_string(hueKey(mac, slot));
}

static void errorIs(const HueWriteReply& reply, unsigned type) {
  StaticJsonDocument<2048> response;
  assert(!deserializeJson(response, reply.json));
  assert(response.is<JsonArray>());
  assert(response[0]["error"]["type"].as<unsigned>() == type);
  assert(!reply.accepted);
}

int main() {
  const uint32_t mac = 0x112233; // Synthetic bridge identity.
  for (unsigned i=0;i<kStations;++i) { ids[i]=i<4?i+1:i+2; configured[i]=true; }
  // Exercise the production discovery serializer, not only PUT routing. IDs 6–9
  // are the owner's reported failing setup after removing/re-adding stations.
  const char* bridgeMac = "AA:BB:CC:11:22:33";
  std::set<std::string> uniqueIds;
  DynamicJsonDocument discovery(16384);
  JsonObject collectionJson = discovery.to<JsonObject>();
  StationState asleep;
  recordRead(asleep, 0, 1000, "");
  for (uint32_t id = 1; id <= 17; ++id) {
    if (id == kRetiredGroupId) continue;
    StationConfig config; config.id = id;
    std::strcpy(config.name, "Station");
    JsonObject light = collectionJson.createNestedObject(std::to_string(hueKey(mac, id)));
    writeHueLight(light, config, asleep, 1100, bridgeMac, "test-version", false);
    const std::string unique = light["uniqueid"].as<std::string>();
    assert(std::regex_match(unique, std::regex("([0-9A-F]{2}:){5}[0-9A-F]{2}-[0-9A-F]{2}-00:11")));
    char expected[48];std::snprintf(expected,sizeof(expected),"%s-%02X-00:11",bridgeMac,static_cast<unsigned>(id));
    assert(unique == expected && uniqueIds.insert(unique).second);
    assert(light["state"]["on"].is<bool>() && !light["state"]["on"].as<bool>());
    assert(light["state"]["reachable"].as<bool>());
    assert(light["name"] == "Station" && light["modelid"] == "HASS321");
  }
  assert(collectionJson.size() == 16);
  assert(!collectionJson.containsKey(std::to_string(hueKey(mac, 5))));
  assert(hueUniqueId(bridgeMac, 6) == "AA:BB:CC:11:22:33-06-00:11");
  assert(hueUniqueId(bridgeMac, 10) == "AA:BB:CC:11:22:33-0A-00:11");
  assert(hueUniqueId(bridgeMac, 255) == "AA:BB:CC:11:22:33-FF-00:11");
  for (uint32_t id : {256U, 257U, 65536U, kMaxStationId}) {
    const auto unique = hueUniqueId(bridgeMac,id);
    assert(std::regex_match(unique,std::regex("([0-9A-F]{2}:){7}[0-9A-F]{2}-01")));
    assert(uniqueIds.insert(unique).second);
  }
  // Serialize and parse after temporary buffers/strings leave scope.
  std::string wire;serializeJson(discovery,wire);
  DynamicJsonDocument parsed(16384);assert(!deserializeJson(parsed,wire));
  assert(parsed[std::to_string(hueKey(mac,6))]["uniqueid"] == "AA:BB:CC:11:22:33-06-00:11");
  discovery.clear();StationConfig config;config.id=6;
  request(asleep,Target::On,1100);
  writeHueLight(discovery.to<JsonObject>(),config,asleep,1101,bridgeMac,"test-version",true);
  assert(discovery["on"].as<bool>() && discovery["reachable"].as<bool>());
  assert(!discovery.containsKey("uniqueid") && asleep.observed==0 && asleep.observedMs==1000);
  assert(!parseHueLightRoute("/api/status", mac).matched);
  assert(!parseHueLightRoute("/api/test-user/config", mac).matched);
  assert(parseHueLightRoute("/api//lights", mac).matched);
  assert(!parseHueLightRoute("/api//lights", mac).valid);
  auto collection = parseHueLightRoute("/api/test-user/lights", mac);
  assert(collection.matched && collection.valid && collection.slot == -1);
  assert(parseHueLightRoute("/api/test-user/lights/", mac).valid);
  errorIs(handleHueWrite(collection, "{\"on\":true}", enqueue), 3);

  for (uint32_t slot : ids) {
    const auto route = parseHueLightRoute((uri(mac, slot) + "/state").c_str(), mac);
    assert(route.valid && route.slot == static_cast<int>(slot) && route.stateOnly);
    for (bool on : {true, false}) {
      const auto reply = handleHueWrite(route, on ? "{\"on\":true}" : "{\"on\":false}", enqueue);
      assert(reply.accepted && reply.on == on);
      assert(lastSlot == static_cast<int>(slot));
      assert(lastTarget == (on ? Target::On : Target::Sleep));
      StaticJsonDocument<512> response;
      assert(!deserializeJson(response, reply.json));
      assert(response.size() == 1);
      const auto success = response[0]["success"].as<JsonObjectConst>();
      const auto expected = "/lights/" + std::to_string(route.key) + "/state/on";
      assert(success.size() == 1 && success[expected].is<bool>());
      assert(success[expected].as<bool>() == on);
      assert(!success.containsKey("/lights/1/state/"));
    }
  }

  const auto route = parseHueLightRoute((uri(mac, 1) + "/state").c_str(), mac);
  unsigned before = queued;
  errorIs(handleHueWrite(route, "not JSON", enqueue), 2);
  errorIs(handleHueWrite(route, nullptr, enqueue), 2);
  errorIs(handleHueWrite(route, "[]", enqueue), 2);
  errorIs(handleHueWrite(route, "{}", enqueue), 5);
  for (auto body : {"{\"on\":1}", "{\"on\":\"true\"}", "{\"on\":null}"})
    errorIs(handleHueWrite(route, body, enqueue), 7);
  errorIs(handleHueWrite(route, "{\"unrelated\":true}", enqueue), 6);
  errorIs(handleHueWrite(route, std::string(513, ' ').c_str(), enqueue), 2);
  errorIs(handleHueWrite(route, "{\"a\":0,\"b\":0,\"c\":0,\"d\":0,\"e\":0,\"f\":0,\"g\":0,\"h\":0,\"on\":true}", enqueue), 2);
  assert(queued == before); // No command on malformed values or unrelated true.

  const auto mixed = handleHueWrite(route, "{\"on\":false,\"unrelated\":true}", enqueue);
  assert(mixed.accepted && !mixed.on && mixed.unsupported == 1 && lastTarget == Target::Sleep);
  StaticJsonDocument<1024> response;
  assert(!deserializeJson(response, mixed.json));
  assert(response.size() == 2);
  assert(response[1]["error"]["type"].as<unsigned>() == 6);

  for (const auto& bad : {uri(mac, 5), uri(mac, 1) + "x",
                         uri(mac, 1) + "/state/extra", std::string("/api/u/lights/2147483648/state"),
                         std::string("/api/u/lights/-1/state"), std::string("/api/u/lights/999999999999999999999")}) {
    const auto invalid = parseHueLightRoute(bad.c_str(), mac);
    assert(invalid.matched && !invalid.valid);
    errorIs(handleHueWrite(invalid, "{\"on\":true}", enqueue), 3);
  }
  assert(parseHueLightRoute((uri(0xffffff, 4) + "/state").c_str(), 0xffffff).valid);
  configured[0] = false;
  errorIs(handleHueWrite(route, "{\"on\":true}", enqueue), 3);
  configured[0] = true;

  // Retired group 5 is never a new station; repeated individual commands coalesce.
  const auto group = parseHueLightRoute((uri(mac, 5) + "/state").c_str(), mac);
  errorIs(handleHueWrite(group, "{\"on\":true}", enqueue), 3);
  assert(handleHueWrite(route, "{\"on\":true}", enqueue).accepted);
  const auto generation = states[0].generation;
  states[0].pending = false; states[0].busy = true;
  assert(handleHueWrite(route, "{\"on\":true}", enqueue).accepted);
  assert(states[0].generation == generation && !states[0].pending && states[0].busy);

  // A prompt acceptance is independent from the measured result of BLE work.
  states[0] = StationState{};
  recordRead(states[0], 0, 1000, "");
  const auto accepted = handleHueWrite(route, "{\"on\":true}", enqueue);
  assert(accepted.accepted && states[0].pending);
  assert(!measuredOn(states[0]) && freshMeasurement(states[0], 1001));
  assert(hueOn(states[0], 1001) && hueTargetPending(states[0], 1001));
  assert(states[0].observedMs == 1000);
  finish(states[0], states[0].generation, true, 9, 2000, "");
  assert(measuredOn(states[0]) && freshMeasurement(states[0], 2001));
  std::cout << "Hue API passed: IDs, boolean ON/SLEEP ACKs, 16 individual devices, retired group, invalid requests and measured-state separation.\n";
}
