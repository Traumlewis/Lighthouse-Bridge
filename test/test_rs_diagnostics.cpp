#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#define DRAM_ATTR
#define IRAM_ATTR
static const int ESP_OK=0;
using Writer=int(*)(const char*,va_list);
using Hook=void(*)(size_t,uint32_t,const char*);
static Writer sdkWriter=nullptr;
static Hook failureHook=nullptr;
static int hookResult=0, reports=0, romCalls=0;
static int64_t timeUs=0;
static char message[192];
static int64_t esp_timer_get_time() { return timeUs; }
static int esp_rom_vprintf(const char* format,va_list args) {
  ++romCalls;return vsnprintf(message,sizeof(message),format,args);
}
static int esp_rom_printf(const char* format,...) {
  ++reports;va_list args;va_start(args,format);
  const int n=esp_rom_vprintf(format,args);va_end(args);return n;
}
static Writer esp_log_set_vprintf(Writer writer) { Writer old=sdkWriter;sdkWriter=writer;return old; }
static int heap_caps_register_failed_alloc_callback(Hook hook) { failureHook=hook;return hookResult; }
#include "rs_diagnostics.h"
static void sdkLog(const char* format,...) {
  va_list args;va_start(args,format);sdkWriter(format,args);va_end(args);
}
int main() {
  assert(rsDiagnostics::install());
  assert(sdkWriter==esp_rom_vprintf && failureHook);
  sdkLog("SDK error %d",574);assert(!strcmp(message,"SDK error 574") && romCalls==1);
  // The name pointer is deliberately invalid: the hook must not dereference it.
  timeUs=100000;failureHook(8192,0x804,reinterpret_cast<const char*>(1));
  assert(reports==1 && strstr(message,"bytes=8192 caps=0x804 count=1"));
  failureHook(64,0x408,nullptr);assert(reports==1 && rsDiagnostics::failures.load()==2);
  timeUs=1100000;failureHook(96,0x408,nullptr);assert(reports==2);
  rsDiagnostics::lastReportMs.store(UINT32_MAX-500);timeUs=600000;
  failureHook(128,0x408,nullptr);assert(reports==3); // millis wrap
  hookResult=-1;assert(!rsDiagnostics::install());
  puts("rs-matter SDK logging: ROM routing, numeric-only OOM hook, throttling, wrap and registration errors passed.");
}
