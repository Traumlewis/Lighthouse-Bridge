#pragma once
#include "scan_capture.h"
#ifdef USING_NIMBLE_ARDUINO_HEADERS
#include "nimble/nimble/host/include/host/ble_gap.h"
#include "nimble/nimble/host/include/host/ble_hs_id.h"
#else
#include <host/ble_gap.h>
#include <host/ble_hs_id.h>
#endif
static_assert(BLE_HCI_ADV_RPT_EVTYPE_ADV_IND == 0 && BLE_HCI_ADV_RPT_EVTYPE_DIR_IND == 1 &&
              BLE_HCI_ADV_RPT_EVTYPE_SCAN_IND == 2 && BLE_HCI_ADV_RPT_EVTYPE_NONCONN_IND == 3 &&
              BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP == 4,"Bind legacy report classification to pinned HCI values");

// The worker owns the scan lifetime. The NimBLE host callback only copies raw
// reports to worker-owned fixed storage; no result objects/vectors are created.
struct MatterScanCapture {
  FoundStation* entries = nullptr;
  const char* assigned = nullptr;
  RecoveryCapture recovery;
  unsigned count = 0;
  bool truncated = false;
  int reason = 0;
  std::atomic<bool> complete{false};
};
int matterScanEvent(ble_gap_event* event, void* argument) {
  auto& capture = *static_cast<MatterScanCapture*>(argument);
  if (event->type == BLE_GAP_EVENT_DISC) {
    const auto& report = event->disc;
    if (capture.entries) captureDiscoveryReport(capture.entries,kStations,capture.count,capture.truncated,
      report.addr.val,report.addr.type,report.rssi,report.data,report.length_data);
    else capture.recovery.capture(capture.assigned,report.addr.val,report.addr.type,report.event_type,report.rssi);
  } else if (event->type == BLE_GAP_EVENT_DISC_COMPLETE) {
    capture.reason = event->disc_complete.reason;
    capture.complete.store(true,std::memory_order_release);
  }
  return 0;
}
int runMatterScan(MatterScanCapture& capture, bool active, int32_t durationMs) {
  ble_gap_disc_params parameters{};
  parameters.itvl = 97*16/10; parameters.window = 67*16/10;
  parameters.passive = !active; parameters.filter_duplicates = 1;
  uint8_t ownType = 0;
  int code = ble_hs_id_infer_auto(0,&ownType);
  if (!code) code = ble_gap_disc(ownType,durationMs,&parameters,matterScanEvent,&capture);
  if (code) return code; // No callback owns this context if start was rejected.
  while (!capture.complete.load(std::memory_order_acquire)) vTaskDelay(pdMS_TO_TICKS(20));
  return capture.reason;
}
