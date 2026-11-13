#pragma once
#include <stdint.h>
namespace bridge {
inline bool startupHttpDue(uint32_t now,uint32_t started) { return now-started >= 3000; }
// Reserve the unchanged worker stack before Matter can fragment the heap.
// Web recovery must survive either failure; no radio work before release.
template<class Services> void startMatterServices(Services& services) {
  const bool reserved = services.reserveWorker();
  services.startWeb();
  if (!reserved) { services.failed("BLE_WORKER_FAILED"); return; }
  if (!services.startBle()) { services.failed("BLE_INIT_FAILED"); return; }
  if (!services.startMatter()) { services.failed("MATTER_START_FAILED"); return; }
  services.releaseWorker();
}
} // namespace bridge
