#include "bridge_core.h"
#include <cassert>
#include <iostream>

int main() {
  using namespace bridge;
  Target target = Target::On;
  assert(parseTarget("sleep", target) && static_cast<uint8_t>(target) == 0x00);
  assert(parseTarget("on", target) && static_cast<uint8_t>(target) == 0x01);
  for (auto invalid : {"standby", "off", "0", "2", "ON", "", "sleep now"})
    assert(!parseTarget(invalid, target));
  assert(!parseTarget(nullptr, target));
  assert(std::strcmp(observedName(0x01), "booting") == 0);
  assert(std::strcmp(observedName(0x0b), "on") == 0);
  assert(std::strcmp(observedName(0x02), "standby") == 0);
  char mac[18]{};
  assert(normalizeMac("aa:bb:cc:dd:ee:ff", mac));
  assert(std::strcmp(mac, "AA:BB:CC:DD:EE:FF") == 0);
  for (auto invalid : {"", "aabbccddeeff", "GG:BB:CC:DD:EE:FF", "AA-BB-CC-DD-EE-FF"})
    assert(!normalizeMac(invalid, mac));
  assert(validName("Basisstation 1"));
  for (auto invalid : {"", "   ", "a\"b", "a\\b", "a<b", "a\nb", "a&b"})
    assert(!validName(invalid));
  StationState state;
  assert(!state.hasRequest && !state.pending); // Boot never sends SLEEP.
  request(state, Target::On);
  auto old = state.generation;
  state.pending = false;
  state.busy = true;
  request(state, Target::Sleep);
  finish(state, old, true, 0x08, 10, "");
  assert(state.pending && !state.sent && state.requested == Target::Sleep);
  state.pending = false;
  finish(state, state.generation, false, -1, 20, "Keine Verbindung");
  assert(!state.sent && std::strcmp(state.error, "Keine Verbindung") == 0);
  assert(state.observed == 0x08); // Failed read does not invent a state.
  request(state, Target::Sleep);
  state.pending = false;
  finish(state, state.generation, true, 0x00, 30, "");
  assert(state.sent && state.lastSentMs == 30 && state.observedMs == 30);
  std::cout << "Core tests passed (ON/SLEEP, validation, superseded commands, failures).\n";
}
