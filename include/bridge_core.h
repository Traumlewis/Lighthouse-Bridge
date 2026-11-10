#pragma once
#include <cstdint>
#include <cstring>
#include <cctype>

namespace bridge {
constexpr unsigned kStations = 4;
constexpr char kService[] = "00001523-1212-efde-1523-785feabcd124";
constexpr char kPower[] = "00001525-1212-efde-1523-785feabcd124";
enum class Target : uint8_t { Sleep = 0x00, On = 0x01 };

inline bool isConnectableLegacyAdvertisement(uint8_t eventType) {
  // Legacy HCI reports use enum values, not extended-advertising bit flags:
  // ADV_IND = 0, ADV_DIRECT_IND = 1. Scan responses are not advertisements.
  return eventType == 0 || eventType == 1;
}

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
  bool reading = false;
  bool reachable = false;
  uint8_t readFailures = 0;
  int bleError = 0;
  uint32_t lastReadAttemptMs = 0;
  uint32_t requestedMs = 0;
  uint32_t generation = 0;
  uint32_t lastSentMs = 0;
  uint32_t observedMs = 0;
  int observed = -1;
  char error[96] = {};
};
inline bool measuredOn(const StationState& state) {
  return state.observed == 0x0b || state.observed == 0x01 ||
         state.observed == 0x08 || state.observed == 0x09;
}
inline bool freshMeasurement(const StationState& state, uint32_t now) {
  return state.reachable && state.observed >= 0 && now - state.observedMs < 60000;
}
constexpr uint32_t kHueTargetWindowMs = 30000;
inline bool hueTargetPending(const StationState& state, uint32_t now) {
  // Owner-approved Alexa compatibility window. Never change measured data or
  // reachability; expiry/command completion immediately restores measured ON.
  return state.hasRequest && (state.pending || state.busy) &&
         now - state.requestedMs < kHueTargetWindowMs;
}
inline bool hueOn(const StationState& state, uint32_t now) {
  return hueTargetPending(state, now) ? state.requested == Target::On : measuredOn(state);
}
inline uint32_t readInterval(const StationState& state) {
  if (state.readFailures == 1) return 5000; // Quickly verify a transient failure.
  if (!state.reachable) return 30000;
  return state.observed == 1 || state.observed == 8 || state.observed == 9 ? 2000 : 15000;
}
inline void recordRead(StationState& state, int observed, uint32_t now, const char* error) {
  state.reading = false;
  state.lastReadAttemptMs = now;
  if (observed >= 0) {
    state.readFailures = 0; state.reachable = true;
    state.observed = observed; state.observedMs = now;
  } else {
    if (state.readFailures < 255) ++state.readFailures;
    // Keep a recent measurement usable after one failed read cycle, but never
    // invent a measurement or refresh its timestamp. The UI labels this retry.
    if (state.readFailures >= 2 || state.observed < 0) state.reachable = false;
  }
  std::strncpy(state.error, error ? error : "", sizeof(state.error) - 1);
  state.error[sizeof(state.error) - 1] = '\0';
}
inline void finishRead(StationState& state, uint32_t generation, int observed,
                       uint32_t now, const char* error, int bleError) {
  state.reading = false;
  if (state.generation != generation) return;
  recordRead(state, observed, now, error);
  state.bleError = bleError;
}
inline void request(StationState& state, Target target, uint32_t now = 0) {
  // Alexa may address both the virtual group and its individual stations.
  // A duplicate of the active goal must not invalidate or repeat that work.
  if (state.hasRequest && state.requested == target && (state.pending || state.busy)) return;
  state.requested = target;
  state.hasRequest = true;
  state.requestedMs = now;
  state.pending = true;
  state.sent = false;
  state.error[0] = '\0';
  ++state.generation;
}
// A result from an older command must not clear a newer pending request.
inline void finish(StationState& state, uint32_t generation, bool sent,
                   int observed, uint32_t now, const char* error) {
  state.busy = false;
  state.lastReadAttemptMs = now;
  state.reachable = observed >= 0;
  state.readFailures = observed >= 0 ? 0 : (sent ? 1 : 2);
  if (observed >= 0) { state.observed = observed; state.observedMs = now; }
  if (generation != state.generation) return;
  state.sent = sent;
  if (sent) state.lastSentMs = now;
  std::strncpy(state.error, error ? error : "", sizeof(state.error) - 1);
  state.error[sizeof(state.error) - 1] = '\0';
}
} // namespace bridge

