#pragma once
#include <cstdint>
#include <limits>
namespace bridge {
// Numeric protocol metadata only. Never store packet data or peer identities.
struct TasmotaPairingDiagnostics {
  uint32_t received = 0, sendFailures = 0, unacked = 0, errors = 0, mdnsErrors = 0, completed = 0;
  uint32_t generalCommands = 0, credentialCommands = 0;
  uint16_t secureOpcode = 0xffff, interactionOpcode = 0xffff;
  uint16_t fabrics = 0;
  bool windowOpen = false, advertised = false;
  static void increment(uint32_t& value) {
    if (value != std::numeric_limits<uint32_t>::max()) ++value;
  }
  void record(unsigned event, unsigned first, unsigned second) {
    switch (event) {
      case 0: increment(received); break;
      case 1:
        if (second > 0xff || second == 0x10) break; // Standalone ACK is not a commissioning step.
        if (first == 0) secureOpcode = second;
        if (first == 1) interactionOpcode = second;
        break;
      case 2:
        if (second >= 32) break;
        if (first == 0x30) generalCommands |= uint32_t(1) << second;
        if (first == 0x3e) credentialCommands |= uint32_t(1) << second;
        break;
      case 3: increment(mdnsErrors); break;
      case 4: increment(sendFailures); break;
      case 5: increment(unacked); break;
      case 6: increment(completed); break;
      case 7: increment(errors); break;
      case 8:
        if (first > 3 || second > 0xffff) break;
        windowOpen = first & 1; advertised = first & 2; fabrics = second;
        break;
    }
  }
};
}
