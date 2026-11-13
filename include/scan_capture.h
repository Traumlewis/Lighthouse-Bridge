#pragma once
#include "bridge_core.h"
#include <cstdio>

namespace bridge {
inline void reportMac(const uint8_t* address, char* mac) {
  std::snprintf(mac,18,"%02X:%02X:%02X:%02X:%02X:%02X",
    address[5],address[4],address[3],address[2],address[1],address[0]);
}
// Parse complete/shortened local-name AD fields without allocating a string.
inline bool lighthouseReportName(const uint8_t* data, size_t size, char* name, size_t capacity) {
  const uint8_t* selected = nullptr; size_t length = 0;
  for (size_t offset = 0; offset < size;) {
    const size_t field = data[offset];
    if (!field) break;
    if (field >= size-offset) return false;
    const uint8_t type = data[offset+1];
    if (type == 9 || (type == 8 && !selected)) {
      selected = data+offset+2; length = field-1;
    }
    offset += field+1;
  }
  if (!selected || length < 4 || std::memcmp(selected,"LHB-",4) || !capacity) return false;
  const size_t count = length < capacity-1 ? length : capacity-1;
  std::memcpy(name,selected,count); name[count] = '\0'; return true;
}
template<class Entry> void captureDiscoveryReport(Entry* entries, size_t capacity,
    unsigned& count, bool& truncated, const uint8_t* address, uint8_t addressType,
    int rssi, const uint8_t* data, size_t size) {
  char name[48], mac[18];
  if (!lighthouseReportName(data,size,name,sizeof(name))) return;
  reportMac(address,mac);
  unsigned index = 0;
  while (index < count && std::strcmp(entries[index].mac,mac)) ++index;
  if (index == count) {
    if (count == capacity) { truncated = true; return; }
    ++count;
  }
  auto& entry = entries[index];
  std::strcpy(entry.name,name); std::strcpy(entry.mac,mac);
  entry.type = addressType; entry.rssi = rssi;
}
struct RecoveryCapture {
  bool matched = false, seen = false;
  uint8_t type = 0, addressType = 0;
  int rssi = 0;
  uint32_t reports = 0;
  void capture(const char* assigned, const uint8_t* address, uint8_t addrType, uint8_t eventType, int signal) {
    if (reports != UINT32_MAX) ++reports;
    char mac[18]; reportMac(address,mac);
    if (std::strcmp(mac,assigned)) return;
    matched = true;
    const bool connectable = isConnectableLegacyAdvertisement(eventType);
    if (seen && !connectable) return;
    seen |= connectable; type = eventType; addressType = addrType; rssi = signal;
  }
};
} // namespace bridge
