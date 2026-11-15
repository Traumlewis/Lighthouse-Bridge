#pragma once
#include <ArduinoJson.h>
#include <cstring>

namespace bridge {
// Optional login name: a bounded ASCII identifier, never HTML or a credential URL.
inline bool validWebUsername(const char* value) {
  if (!value || !*value || std::strlen(value) > 32) return false;
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value); *p; ++p)
    if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
          (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' || *p == '.')) return false;
  return true;
}
inline const char* storedWebUsername(JsonVariantConst input) {
  return input["username"] | "";
}
inline bool parseWebUsername(JsonVariantConst input, bool& enabled, const char*& name) {
  if (!input["username_enabled"].is<bool>()) return false;
  enabled = input["username_enabled"].as<bool>();
  name = "";
  if (input.containsKey("username") && !input["username"].is<const char*>()) return false;
  if (enabled) {
    name = input["username"].as<const char*>();
    if (!validWebUsername(name)) return false;
  }
  return true;
}
} // namespace bridge
