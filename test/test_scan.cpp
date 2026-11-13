#include "status_api.h"
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <new>
#include <cstdio>
using namespace bridge;
using FoundStation = StatusDiscovery;
void vTaskDelay(unsigned);
#define pdMS_TO_TICKS(ms) (ms)
#include "matter_scan.h"

size_t newCalls = 0;
void* operator new(size_t size) {
  ++newCalls; if (void* result = std::malloc(size)) return result;
  throw std::bad_alloc();
}
void operator delete(void* pointer) noexcept { std::free(pointer); }
int idError = 0, startError = 0, endReason = 0, starts = 0, waits = 0;
bool passive = false;
int32_t duration = 0;
int(*callback)(ble_gap_event*,void*) = nullptr;
void* context = nullptr;
int ble_hs_id_infer_auto(int privacy,uint8_t* type) {
  assert(!privacy); *type = 1; return idError;
}
int ble_gap_disc(uint8_t own,int32_t time,const ble_gap_disc_params* params,
                 int(*handler)(ble_gap_event*,void*),void* argument) {
  ++starts; assert(own == 1 && params->itvl == 155 && params->window == 107);
  assert(params->filter_duplicates && !params->limited && !params->filter_policy);
  passive = params->passive; duration = time;
  if (!startError) { callback = handler; context = argument; }
  return startError;
}
const uint8_t named[] = {9,9,'L','H','B','-','t','e','s','t'};
const uint8_t foreign[] = {5,9,'t','e','s','t'};
void vTaskDelay(unsigned ms) {
  assert(ms == 20 && callback); ++waits;
  ble_gap_event event{}; event.type = BLE_GAP_EVENT_DISC;
  event.disc.data = named; event.disc.length_data = sizeof(named);
  event.disc.addr.type = 1; event.disc.rssi = -60;
  event.disc.data = foreign; event.disc.length_data = sizeof(foreign);
  for (unsigned i = 0; i < 4000; ++i) {
    event.disc.addr.val[0] = i%250; callback(&event,context);
  }
  event.disc.data = named; event.disc.length_data = sizeof(named);
  for (unsigned i = 0; i < 4000; ++i) {
    event.disc.addr.val[0] = i%20; callback(&event,context);
  }
  event.type = BLE_GAP_EVENT_DISC_COMPLETE; event.disc_complete.reason = endReason;
  callback(&event,context); callback = nullptr; context = nullptr;
}
int main() {
  const size_t baseline = newCalls;
  FoundStation entries[16]{};
  MatterScanCapture scan; scan.entries = entries;
  assert(runMatterScan(scan,true,5000) == 0);
  assert(!passive && duration == 5000 && waits == 1);
  assert(scan.count == 16 && scan.truncated);
  assert(!std::strcmp(entries[0].name,"LHB-test"));
  assert(!std::strcmp(entries[0].mac,"00:00:00:00:00:00"));
  assert(!std::strcmp(entries[15].mac,"00:00:00:00:00:0F"));
  assert(newCalls == baseline); // Flood/complete path cannot copy a heap vector.
  MatterScanCapture recovery; recovery.assigned = "00:00:00:00:00:03";
  assert(runMatterScan(recovery,false,2000) == 0);
  assert(passive && duration == 2000 && recovery.recovery.seen);
  assert(newCalls == baseline);
  startError = 17; MatterScanCapture failed;
  assert(runMatterScan(failed,true,5000) == 17 && !failed.complete && !callback);
  const int attempts = starts;
  idError = 2; MatterScanCapture noId;
  assert(runMatterScan(noId,true,5000) == 2 && starts == attempts);
  idError = startError = 0; endReason = 19;
  MatterScanCapture interrupted; interrupted.entries = entries;
  assert(runMatterScan(interrupted,true,5000) == 19 && interrupted.complete);

  char name[48];
  assert(!lighthouseReportName(nullptr,0,name,sizeof(name)));
  const uint8_t malformed[] = {20,9,'L','H','B','-'};
  assert(!lighthouseReportName(malformed,sizeof(malformed),name,sizeof(name)));
  const uint8_t other[] = {5,9,'t','e','s','t'};
  assert(!lighthouseReportName(other,sizeof(other),name,sizeof(name)));
  const uint8_t both[] = {5,8,'L','H','B','-',9,9,'L','H','B','-','f','u','l','l'};
  assert(lighthouseReportName(both,sizeof(both),name,sizeof(name)) && !std::strcmp(name,"LHB-full"));
  uint8_t longName[80]; std::memset(longName,'x',sizeof(longName));
  longName[0] = 79; longName[1] = 9; std::memcpy(longName+2,"LHB-",4);
  assert(lighthouseReportName(longName,sizeof(longName),name,sizeof(name)) && std::strlen(name) == 47);
  const uint8_t address[6] = {1,2,3,4,5,6};
  for (uint8_t type = 0; type < 6; ++type) {
    RecoveryCapture result;
    result.capture("06:05:04:03:02:01",address,1,type,-40);
    assert(result.matched && result.seen == (type < 2));
  }
  RecoveryCapture wrong;
  wrong.capture("06:05:04:03:02:02",address,1,0,-40);
  assert(!wrong.seen && !wrong.matched);
  assert(newCalls == baseline);
  puts("Matter raw scans passed: allocation-free flood, capacity/dedup, AD parser, recovery MAC/type, worker wait and start/end errors.");
}
