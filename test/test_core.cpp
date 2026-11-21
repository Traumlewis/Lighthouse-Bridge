#include "bridge_core.h"
#include <cassert>
#include <iostream>

int main() {
  using namespace bridge;
  assert(!matchesTarget(Target::Sleep, 2)); // STANDBY never confirms SLEEP.
  assert(!matchesTarget(Target::On, 1)); // Starting never confirms steady ON.
  // Writing without response is not a physical-state confirmation.
  StationState verification;
  assert(std::strcmp(commandStatus(verification, 0), "none") == 0);
  request(verification, Target::On, 100);
  assert(std::strcmp(commandStatus(verification, 101), "queued") == 0);
  verification.pending = false; verification.busy = true;
  assert(std::strcmp(commandStatus(verification, 102), "sending") == 0);
  finish(verification, verification.generation, true, 0, 200, "");
  assert(verification.sent && !verification.commandConfirmed);
  assert(std::strcmp(commandStatus(verification, 201), "verifying") == 0);
  assert(readInterval(verification, 201) == 2000 && !hueTargetPending(verification, 201));
  assert(!hueOn(verification, 201)); // Do not extend the approved Alexa window.
  finishRead(verification, verification.generation, 9, 1000, "", 0);
  assert(!verification.commandConfirmed && measuredOn(verification)); // Starting is not steady ON.
  finishRead(verification, verification.generation, 11, 2000, "", 0);
  assert(verification.commandConfirmed && !awaitingConfirmation(verification, 2001));
  assert(readInterval(verification, 2001) == 15000);
  assert(std::strcmp(commandStatus(verification, 2001), "confirmed") == 0);
  recordRead(verification, 0, 3000, ""); // Later external switching does not rewrite command history.
  assert(verification.commandConfirmed && !measuredOn(verification));
  request(verification, Target::Sleep, 4000);
  assert(!verification.commandConfirmed);
  verification.pending = false; verification.busy = true;
  finish(verification, verification.generation, true, 0, 4100, "");
  assert(verification.commandConfirmed && std::strcmp(commandStatus(verification, 4101), "confirmed") == 0);
  request(verification, Target::On, 5000);
  verification.pending = false; verification.busy = true;
  finish(verification, verification.generation, true, -1, 5100, "BLE_READ_FAILED");
  assert(awaitingConfirmation(verification, 5101) && !freshMeasurement(verification, 5101));
  assert(readInterval(verification, 5101) == 2000);
  assert(awaitingConfirmation(verification, 35099) && !awaitingConfirmation(verification, 35100));
  finishRead(verification, verification.generation, 11, 35100, "", 0);
  assert(!verification.commandConfirmed && std::strcmp(commandStatus(verification, 35101), "unconfirmed") == 0);
  assert(measuredOn(verification) && freshMeasurement(verification, 35101));
  request(verification, Target::Sleep, 36000);
  verification.pending = false; verification.busy = true;
  finish(verification, verification.generation, false, -1, 36100, "BLE_CONNECT_FAILED");
  assert(std::strcmp(commandStatus(verification, 36101), "failed") == 0);
  assert(!awaitingConfirmation(verification, 36101));
  request(verification, Target::On, 0xffffff00U);
  verification.pending = false; verification.busy = true;
  finish(verification, verification.generation, true, 9, 0xfffffff0U, "");
  assert(awaitingConfirmation(verification, 0x20));
  finishRead(verification, verification.generation, 11, 0x20, "", 0);
  assert(verification.commandConfirmed);
  // Alexa may briefly expose an accepted in-flight goal; measurements stay real.
  StationState hue;
  recordRead(hue, 0, 100, "");
  assert(!hueTargetPending(hue, 150) && !hueOn(hue, 150));
  request(hue, Target::On, 200);
  const auto firstGeneration = hue.generation;
  assert(hueTargetPending(hue, 201) && hueOn(hue, 201));
  assert(!measuredOn(hue) && hue.observedMs == 100 && freshMeasurement(hue, 201));
  request(hue, Target::On, 10000);
  assert(hue.requestedMs == 200 && hue.generation == firstGeneration);
  assert(hueTargetPending(hue, 30199) && !hueTargetPending(hue, 30200));
  assert(!hueOn(hue, 30200)); // No indefinite target if worker is delayed/stuck.
  hue.pending = false; hue.busy = true;
  assert(hueOn(hue, 300));
  finish(hue, hue.generation, true, 0, 400, "");
  assert(!hueTargetPending(hue, 401) && !hueOn(hue, 401));
  request(hue, Target::On, 500); // A new request after completion can retry.
  assert(hue.generation != firstGeneration && hue.requestedMs == 500);
  hue.pending = false; hue.busy = true;
  finish(hue, hue.generation, false, -1, 600, "BLE_CONNECT_FAILED");
  assert(!hueTargetPending(hue, 601) && !freshMeasurement(hue, 601));
  recordRead(hue, 11, 700, "");
  request(hue, Target::Sleep, 800);
  assert(measuredOn(hue) && !hueOn(hue, 801));
  const auto sleepGeneration = hue.generation;
  request(hue, Target::On, 900);
  assert(hue.generation != sleepGeneration && hueOn(hue, 901));
  hue.pending = false; hue.busy = true;
  finish(hue, hue.generation, true, 9, 1000, "");
  assert(!hueTargetPending(hue, 1001) && hueOn(hue, 1001));
  request(hue, Target::Sleep, 0xfffffff0U);
  assert(hueTargetPending(hue, 0x20) && !hueOn(hue, 0x20));
  assert(!hueTargetPending(hue, static_cast<uint32_t>(0xfffffff0U + 30000U)));
  StationState noMeasurement;
  request(noMeasurement, Target::On, 10);
  assert(hueOn(noMeasurement, 11) && !freshMeasurement(noMeasurement, 11));
  // Legacy HCI report types are enum values. A bitmask test rejects normal
  // connectable ADV_IND (0) and accepts non-connectable types 3 and 4.
  assert(isConnectableLegacyAdvertisement(0)); // ADV_IND
  assert(isConnectableLegacyAdvertisement(1)); // ADV_DIRECT_IND
  assert(!isConnectableLegacyAdvertisement(2)); // ADV_SCAN_IND
  assert(!isConnectableLegacyAdvertisement(3)); // ADV_NONCONN_IND
  assert(!isConnectableLegacyAdvertisement(4)); // SCAN_RSP
  assert(!isConnectableLegacyAdvertisement(255));
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
  assert(validName("Base station 1"));
  for (auto invalid : {"", "   ", "a\"b", "a\\b", "a<b", "a\nb", "a&b"})
    assert(!validName(invalid));
  StationState state;
  assert(!state.hasRequest && !state.pending); // Boot never sends SLEEP.
  request(state, Target::On);
  auto old = state.generation;
  request(state, Target::On);
  assert(state.generation == old && state.pending);
  state.pending = false;
  state.busy = true;
  request(state, Target::On);
  assert(state.generation == old && !state.pending && state.busy);
  request(state, Target::Sleep);
  finish(state, old, true, 0x08, 10, "");
  assert(state.pending && !state.sent && state.requested == Target::Sleep);
  assert(!state.commandConfirmed);
  state.pending = false;
  finish(state, state.generation, false, -1, 20, "No connection");
  assert(!state.sent && std::strcmp(state.error, "No connection") == 0);
  assert(state.observed == 0x08); // Failed read does not invent a state.
  request(state, Target::Sleep);
  state.pending = false;
  finish(state, state.generation, true, 0x00, 30, "");
  assert(state.sent && state.lastSentMs == 30 && state.observedMs == 30);
  // Background measurements stay separate from requested targets.
  request(state, Target::On);
  recordRead(state, 0x00, 100, "");
  assert(!measuredOn(state));
  assert(freshMeasurement(state, 110));
  recordRead(state, 0x0b, 200, "");
  assert(measuredOn(state) && freshMeasurement(state, 210));
  recordRead(state, -1, 300, "BLE_CONNECT_FAILED");
  assert(state.observed == 0x0b && freshMeasurement(state, 301));
  assert(state.observedMs == 200 && state.readFailures == 1);
  assert(readInterval(state, 301) == 5000);
  recordRead(state, -1, 5300, "BLE_CONNECT_FAILED");
  assert(!freshMeasurement(state, 5301) && state.readFailures == 2);
  assert(state.observedMs == 200 && readInterval(state, 5301) == 30000);
  recordRead(state, 0x0b, 5400, "");
  assert(state.readFailures == 0 && !state.error[0] && freshMeasurement(state, 5401));
  assert(readInterval(state, 5401) == 15000);
  recordRead(state, 0x02, 400, "");
  assert(!measuredOn(state));
  recordRead(state, 0x01, 500, "");
  assert(measuredOn(state));
  assert(!freshMeasurement(state, 60500));
  recordRead(state, 0x00, 0xfffffff0U, "");
  assert(freshMeasurement(state, 10)); // millis wraparound.
  recordRead(state, -1, 20, "BLE_READ_FAILED");
  assert(freshMeasurement(state, 21) && state.observedMs == 0xfffffff0U);
  assert(!freshMeasurement(state, 60000)); // Grace never extends the reading's age.
  StationState neverRead;
  recordRead(neverRead, -1, 100, "BLE_CONNECT_FAILED");
  assert(!freshMeasurement(neverRead, 101)); // No fake initial state.
  for (unsigned i = 0; i < 300; ++i) recordRead(neverRead, -1, 200 + i, "BLE_CONNECT_FAILED");
  assert(neverRead.readFailures == 255); // Saturate instead of reviving after overflow.
  state.pending = false; state.busy = true;
  finish(state, state.generation, true, -1, 70000, "BLE_READ_FAILED");
  assert(!freshMeasurement(state, 70001) && !state.commandConfirmed && readInterval(state, 70001) == 2000);
  StationState replaced;
  replaced.reading = true;
  const auto oldRead = replaced.generation;
  ++replaced.generation; // Reassignment or a newer user command.
  finishRead(replaced, oldRead, 0x0b, 100, "", 0);
  assert(!replaced.reading && replaced.observed == -1);
  finishRead(replaced, replaced.generation, 0x00, 200, "", 0);
  assert(freshMeasurement(replaced, 201) && replaced.observed == 0);
  std::cout << "Core tests passed (ON/SLEEP, validation, superseded commands, failures).\n";
}
