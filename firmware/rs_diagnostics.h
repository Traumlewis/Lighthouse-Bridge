#pragma once
// The caller supplies the public ESP-IDF logging/heap/timer declarations.
#include <atomic>
#include <stdint.h>
#include <stddef.h>
namespace rsDiagnostics {
static std::atomic<uint32_t> failures{0}, lastReportMs{0};
static DRAM_ATTR const char failureFormat[] =
  "rs-matter allocation failed: bytes=%u caps=0x%x count=%u\n";
// The heap hook must never allocate, log via libc or inspect flash strings.
// Retain numeric evidence even if reporting is throttled or the cache is off.
// noinline keeps GCC LTO from discarding this callback's IRAM section.
static void IRAM_ATTR __attribute__((noinline)) allocationFailed(size_t size,uint32_t caps,const char*) {
  const uint32_t count=failures.fetch_add(1,std::memory_order_relaxed)+1;
  const uint32_t now=static_cast<uint32_t>(esp_timer_get_time()/1000);
  uint32_t previous=lastReportMs.load(std::memory_order_relaxed);
  if ((count==1 || now-previous>=1000) &&
      lastReportMs.compare_exchange_strong(previous,now,std::memory_order_relaxed))
    esp_rom_printf(failureFormat,static_cast<unsigned>(size),static_cast<unsigned>(caps),static_cast<unsigned>(count));
}
template<class ConsoleWrite> inline bool install(ConsoleWrite writeConsole) {
  // Warm the shared UART/VFS write lock while RAM is still available. Use the
  // public console write path, not HardwareSerial (which bypasses this lock).
  // Keep the SDK's standard formatter, including float/long-long support.
  static const char ready[] = "rs-matter UART logging prepared.\n";
  const bool consoleReady=writeConsole(ready,sizeof(ready)-1)==sizeof(ready)-1;
  const bool hookReady=heap_caps_register_failed_alloc_callback(allocationFailed)==ESP_OK;
  return consoleReady && hookReady;
}
}
