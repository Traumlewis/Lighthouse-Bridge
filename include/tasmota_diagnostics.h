#pragma once
#include <cstdint>
#include <limits>
namespace bridge {
struct TasmotaNetworkPolicy {
  bool connected = false;
  uint32_t lastRequest = 0;
  bool requestIpv6(uint32_t now, bool up, bool hasIpv6) {
    if (!up) { connected = false; return false; }
    const bool request = !connected || (!hasIpv6 && uint32_t(now-lastRequest) >= 30000);
    connected = true;
    if (request) lastRequest = now;
    return request;
  }
};
// Numeric protocol metadata only. Never store packet data or peer identities.
struct TasmotaPairingDiagnostics {
  uint32_t received = 0, sendFailures = 0, unacked = 0, errors = 0, mdnsErrors = 0, completed = 0;
  uint32_t generalCommands = 0, credentialCommands = 0;
  uint32_t subscriptions = 0, clientErrors = 0;
  unsigned clientStatus = 0, clientLastError = 0;
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
        if (first == 1) { interactionOpcode = second; if (second == 3) increment(subscriptions); }
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
      case 11:
        if (first > 0xff) break;
        clientStatus = first; if (first) { clientLastError = first; increment(clientErrors); } break;
      case 8:
        if (first > 3 || second > 0xffff) break;
        windowOpen = first & 1; advertised = first & 2; fabrics = second;
        break;
    }
  }
};
struct TasmotaImPath {
  uint32_t cluster = 0, id = 0;
  uint16_t endpoint = 0, status = 0x101;
  bool command = false, write = false;
};
struct TasmotaImDiagnostics {
  static constexpr unsigned capacity = 8;
  TasmotaImPath pending[capacity], firstError, lastError;
  unsigned count = 0, next = 0;
  uint32_t reads = 0, readErrors = 0, writes = 0, writeErrors = 0, invokes = 0, invokeErrors = 0, discarded = 0, removeFabricRequests = 0;
  bool hasError = false;
  void record(unsigned event, unsigned endpoint, uint32_t cluster, uint32_t id, unsigned status) {
    if ((event != 9 && event != 10 && event != 12) || endpoint > 0xffff ||
        (status > 0x101 && !(event == 10 && status >= 0x201 && status <= 0x2ff))) return;
    TasmotaImPath path; path.command = event == 10; path.write = event == 12; path.endpoint = endpoint;
    path.cluster = cluster; path.id = id; path.status = status;
    const unsigned previous = (next+capacity-1)%capacity;
    const auto& old = pending[previous];
    if (count && old.status == 0x101 && old.command == path.command && old.write == path.write &&
        old.endpoint == endpoint && old.cluster == cluster && old.id == id) {
      pending[previous] = path;
    } else {
      pending[next] = path; next = (next+1)%capacity;
      if (count < capacity) ++count; else TasmotaPairingDiagnostics::increment(discarded);
    }
    if (status != 0x101) {
      TasmotaPairingDiagnostics::increment(path.write ? writes : (path.command ? invokes : reads));
      if (path.command && cluster == 0x3e && id == 10) TasmotaPairingDiagnostics::increment(removeFabricRequests);
    }
    if (status && status != 0x100 && status != 0x101) {
      TasmotaPairingDiagnostics::increment(path.write ? writeErrors : (path.command ? invokeErrors : readErrors));
      if (!hasError) firstError = path;
      lastError = path; hasError = true;
    }
  }
  const TasmotaImPath& item(unsigned index) const { return pending[(next+capacity-count+index)%capacity]; }
  void clearPending() { count = 0; next = 0; }
};

}
