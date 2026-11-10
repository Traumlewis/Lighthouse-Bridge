#pragma once
#include <cstdint>
#include <cstring>
#include <cctype>

namespace bridge {
constexpr unsigned kStations = 4;
constexpr char kService[] = "00001523-1212-efde-1523-785feabcd124";
constexpr char kPower[] = "00001525-1212-efde-1523-785feabcd124";
enum class Target : uint8_t { Sleep = 0x00, On = 0x01 };

inline bool parseTarget(const char* text, Target& out) {
  if (text && std::strcmp(text, "on") == 0) { out = Target::On; return true; }
  if (text && std::strcmp(text, "sleep") == 0) { out = Target::Sleep; return true; }
  return false;
}
inline const char* targetName(Target target) { return target == Target::On ? "on" : "sleep"; }
inline const char* observedName(int value) {
  switch (value) {
    case 0x00: return "sleep";
    case 0x02: return "standby"; // Read-only: never a command.
    case 0x0b: return "on";
    case 0x01: case 0x08: case 0x09: return "booting";
    default: return "unknown";
  }
}
inline bool normalizeMac(const char* input, char* out) {
  if (!input || std::strlen(input) != 17) return false;
  for (unsigned i = 0; i < 17; ++i) {
    const unsigned char c = input[i];
    if (i % 3 == 2) { if (c != ':') return false; }
    else if (!std::isxdigit(c)) return false;
    out[i] = std::toupper(c);
  }
  out[17] = '\0';
  return true;
}
// Espalexa interpolates names into JSON/XML; reject delimiters it doesn't escape.
inline bool validName(const char* name) {
  if (!name || !*name || std::strlen(name) > 40) return false;
  bool visible = false;
  for (const unsigned char* c = reinterpret_cast<const unsigned char*>(name); *c; ++c) {
    if (*c < 32 || *c == 127 || std::strchr("\"\\<>&", *c)) return false;
    visible |= *c != ' ';
  }
  return visible;
}

struct StationConfig {
  char name[41] = {};
  char mac[18] = {};
  uint8_t addressType = 0;
};
struct StationState {
  Target requested = Target::Sleep;
  bool hasRequest = false;
  bool pending = false;
  bool busy = false;
  bool sent = false;
  uint32_t generation = 0;
  uint32_t lastSentMs = 0;
  uint32_t observedMs = 0;
  int observed = -1;
  char error[96] = {};
};
inline void request(StationState& state, Target target) {
  state.requested = target;
  state.hasRequest = true;
  state.pending = true;
  state.sent = false;
  state.error[0] = '\0';
  ++state.generation;
}
// A result from an older command must not clear a newer pending request.
inline void finish(StationState& state, uint32_t generation, bool sent,
                   int observed, uint32_t now, const char* error) {
  state.busy = false;
  if (observed >= 0) { state.observed = observed; state.observedMs = now; }
  if (generation != state.generation) return;
  state.sent = sent;
  if (sent) state.lastSentMs = now;
  std::strncpy(state.error, error ? error : "", sizeof(state.error) - 1);
  state.error[sizeof(state.error) - 1] = '\0';
}
} // namespace bridge

