#pragma once
namespace bridge {
// Reserve the unchanged worker stack before Matter can fragment the heap.
// Web recovery must survive either failure; no radio work before release.
template<class Services> void startMatterServices(Services& services) {
  const bool reserved = services.reserveWorker();
  services.startWeb();
  if (!reserved) { services.failed("BLE_WORKER_FAILED"); return; }
  services.startMatter();
  if (!services.startBle()) { services.failed("BLE_INIT_FAILED"); return; }
  services.releaseWorker();
}
} // namespace bridge
