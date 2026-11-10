#pragma once
#include "bridge_core.h"
#include <ArduinoJson.h>
#include <string>

namespace bridge {
struct HueLightRoute {
  bool matched = false;
  bool valid = false;
  bool stateOnly = false;
  int slot = -1; // -1 = collection, 0..3 = station, 4 = group.
  uint32_t key = 0;
};

inline HueLightRoute parseHueLightRoute(const char* uri, uint32_t mac24) {
  HueLightRoute route;
  if (!uri || std::strncmp(uri, "/api/", 5)) return route;
  const char* resource = std::strchr(uri + 5, '/');
  if (!resource || std::strncmp(resource, "/lights", 7)) return route;
  route.matched = true;
  if (resource == uri + 5) return route;
  const char* tail = resource + 7;
  if (!*tail || !std::strcmp(tail, "/")) { route.valid = true; return route; }
  if (*tail++ != '/' || *tail < '0' || *tail > '9') return route;
  uint32_t key = 0;
  while (*tail >= '0' && *tail <= '9') {
    const unsigned digit = static_cast<unsigned>(*tail++ - '0');
    if (key > (0x7fffffffU - digit) / 10) return route;
    key = key * 10 + digit;
  }
  if (*tail) {
    if (std::strcmp(tail, "/state")) return route;
    route.stateOnly = true;
  }
  if ((key >> 7) != mac24 || (key & 127) > kStations) return route;
  route.valid = true;
  route.key = key;
  route.slot = static_cast<int>(key & 127);
  return route;
}

inline void appendHueError(JsonArray replies, unsigned type, const std::string& address, const char* description) {
  auto error = replies.createNestedObject().createNestedObject("error");
  error["type"] = type;
  error["address"] = address;
  error["description"] = description;
}

inline std::string hueError(unsigned type, const char* address, const char* description) {
  StaticJsonDocument<512> doc;
  appendHueError(doc.to<JsonArray>(), type, address, description);
  std::string result;
  serializeJson(doc, result);
  return result;
}

struct HueWriteReply {
  std::string json;
  bool accepted = false;
  bool on = false;
  unsigned unsupported = 0;
};

// Queue-only acknowledgement: no BLE call or invented measurement belongs here.
// The callback returns false when no configured station can accept the request.
inline HueWriteReply handleHueWrite(const HueLightRoute& route, const char* body,
                                   bool (*queue)(int, Target)) {
  HueWriteReply reply;
  if (!route.valid || route.slot < 0 || !route.stateOnly) {
    reply.json = hueError(3, "/lights", "resource not available");
    return reply;
  }
  const std::string address = "/lights/" + std::to_string(route.key) + "/state/";
  StaticJsonDocument<768> input;
  if (!body || std::strlen(body) > 512 || deserializeJson(input, body) || !input.is<JsonObject>()) {
    reply.json = hueError(2, address.c_str(), "body contains invalid JSON");
    return reply;
  }
  StaticJsonDocument<2048> output;
  auto responses = output.to<JsonArray>();
  auto object = input.as<JsonObjectConst>();
  if (object.size() > 8) {
    reply.json = hueError(2, address.c_str(), "too many parameters in body");
    return reply;
  }
  if (object.size() == 0) appendHueError(responses, 5, address, "missing parameters in body");
  for (JsonPairConst property : object) {
    const std::string name = property.key().c_str();
    if (name != "on") {
      ++reply.unsupported;
      appendHueError(responses, 6, address + name, "parameter not available");
      continue;
    }
    if (!property.value().is<bool>()) {
      appendHueError(responses, 7, address + "on", "invalid value for parameter on");
      continue;
    }
    reply.on = property.value().as<bool>();
    reply.accepted = queue(route.slot == static_cast<int>(kStations) ? -1 : route.slot,
                           reply.on ? Target::On : Target::Sleep);
    if (reply.accepted) {
      auto success = responses.createNestedObject().createNestedObject("success");
      success[address + "on"] = reply.on;
    } else {
      appendHueError(responses, 3, address, "no configured station available");
    }
  }
  serializeJson(output, reply.json);
  return reply;
}
} // namespace bridge
