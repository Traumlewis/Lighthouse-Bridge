#pragma once
#include "bridge_core.h"
#include <ctime>

namespace bridge {

inline bool canQueueIdentify(const StationState& state, uint32_t now) {
  return freshMeasurement(state, now) && state.readFailures == 0 &&
         matchesTarget(Target::On, state.observed);
}

// Read the actual power state on the same connection immediately before
// writing. The Identify characteristic can itself wake a sleeping station.
template <class Characteristic>
const char* identifyWhileOn(Characteristic* power, Characteristic* light) {
  if (!light || (!light->canWrite() && !light->canWriteNoResponse())) {
    return "IDENTIFY_UNSUPPORTED";
  }
  if (!power || !power->canRead()) return "IDENTIFY_REQUIRES_ON";
  // NimBLE only sets the out timestamp after a successful read. Its return
  // value alone can contain partial data from an unsuccessful transaction.
  constexpr std::time_t unread = static_cast<std::time_t>(-2);
  std::time_t completed = unread;
  const auto value = power->readValue(&completed);
  if (completed == unread || value.size() != 1 ||
      !matchesTarget(Target::On, static_cast<uint8_t>(value[0]))) {
    return "IDENTIFY_REQUIRES_ON";
  }
  const uint8_t flash = 1;
  return light->writeValue(&flash, 1, light->canWrite()) ? "" : "IDENTIFY_FAILED";
}

} // namespace bridge
