// Lighthouse bindings for the pinned GPL-3.0-or-later Tasmota runtime.
#include "tasmota_runtime.h"
#include <esp_partition.h>
#include <esp_heap_caps.h>
#include "erased_flash.h"
#include "tasmota_diagnostics.h"

extern "C" bool lh_tasmota_mount_fs() {
  if (LittleFS.begin(false,"",5,"spiffs")) return true;
  const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"spiffs");
  if (!part || part->address != 0x3d0000 || part->size != 0x30000) return false;
  const auto read = [part](size_t offset,uint8_t* bytes,size_t size) {
    return esp_partition_read(part,offset,bytes,size) == ESP_OK;
  };
  if (!bridge::completelyErasedFlash(read,part->size)) return false;
  // Only a completely erased, unused filesystem may be formatted automatically.
  return LittleFS.begin(true,"",5,"spiffs");
}
extern "C" size_t lh_tasmota_vm_heap() { return berry.vm ? be_gc_memcount(berry.vm) : 0; }
extern "C" void lh_tasmota_pump() {
  if (berry.vm && be_top(berry.vm) == 0) callBerryFastLoop(true);
}
extern "C" bool lh_tasmota_fs_ready() { return ffsp && ufs_type; }
static bridge::TasmotaPairingDiagnostics lh_pairing_diag;
static int lh_native_diag(bvm* vm) {
  if (be_top(vm) == 3 && be_isint(vm,1) && be_isint(vm,2) && be_isint(vm,3))
    lh_pairing_diag.record(be_toint(vm,1),be_toint(vm,2),be_toint(vm,3));
  be_return_nil(vm);
}
static void lh_print_pairing_diag() {
  static uint32_t last = 0;
  if (millis()-last < 15000) return;
  last = millis();
  const auto& d = lh_pairing_diag;
  Serial.printf("Matter pairing diag: rx=%lu secure_opcode=0x%04x im_opcode=0x%04x general_mask=0x%08lx credentials_mask=0x%08lx send_fail=%lu unacked=%lu rx_error=%lu mdns_error=%lu complete=%lu window=%s advertised=%s fabrics=%u ipv6_local=%s free_8bit=%lu\n",
    static_cast<unsigned long>(d.received),d.secureOpcode,d.interactionOpcode,
    static_cast<unsigned long>(d.generalCommands),static_cast<unsigned long>(d.credentialCommands),
    static_cast<unsigned long>(d.sendFailures),static_cast<unsigned long>(d.unacked),
    static_cast<unsigned long>(d.errors),static_cast<unsigned long>(d.mdnsErrors),
    static_cast<unsigned long>(d.completed),d.windowOpen ? "open" : "closed",
    d.advertised ? "yes" : "no",static_cast<unsigned>(d.fabrics),WiFi.linkLocalIPv6() != IPAddress(IPv6) ? "yes" : "no",
    static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_8BIT)));
}
static int lh_native_power(bvm* vm) {
  const bool valid = be_top(vm) == 2 && be_isint(vm,1) && be_isint(vm,2);
  const int endpoint = valid ? be_toint(vm,1) : 0;
  const int command = valid ? be_toint(vm,2) : -1;
  const bool accepted = endpoint > 0 && endpoint < 0xffff && command >= 0 && command <= 2 &&
    lh_queue_power(static_cast<uint16_t>(endpoint),static_cast<unsigned>(command));
  be_pushbool(vm,accepted); be_return(vm);
}
static const char* lh_vm_phase = "waiting";
static bool lh_vm_function(const char* name) {
  lh_vm_phase = name;
  if (!be_getmodule(berry.vm,"matter")) return false;
  if (!be_getmember(berry.vm,-1,name) || !be_isfunction(berry.vm,-1)) {
    be_pop(berry.vm,be_top(berry.vm)); return false;
  }
  be_remove(berry.vm,-2); return true;
}
static bool lh_vm_result(int arguments) {
  const bool ok = be_pcall(berry.vm,arguments) == BE_OK;
  if (!ok) {
    const char* type = be_top(berry.vm) >= 2 && be_isstring(berry.vm,-2) ? be_tostring(berry.vm,-2) : "VM_EXCEPTION";
    size_t length = strlen(type);
    bool safe = length > 0 && length <= 32;
    for (size_t i = 0; i < length && safe; ++i)
      safe = (type[i] >= 'a' && type[i] <= 'z') || (type[i] >= 'A' && type[i] <= 'Z') || type[i] == '_';
    Serial.printf("Tasmota Matter call failed: phase=%s, type=%s\n",lh_vm_phase,safe ? type : "VM_EXCEPTION");
    be_pop(berry.vm,be_top(berry.vm));
  }
  return ok;
}
extern "C" bool lh_tasmota_begin(const char* configuration) {
  if (!berry.vm || !lh_tasmota_fs_ready() || !configuration) return false;
  Settings->flag6.matter_enabled = 1;
  Settings->flag.mqtt_enabled = 0;
  SettingsUpdateText(SET_DEVICENAME,"Lighthouse-Bridge Matter Test");
  WifiSetState(WiFi.status() == WL_CONNECTED);
  be_pop(berry.vm,be_top(berry.vm));
  be_regfunc(berry.vm,"lh_power",lh_native_power);
  be_regfunc(berry.vm,"lh_diag",lh_native_diag);
  const char* dependencies[] = {"json","crypto","mdns","path","introspect","string","math"};
  for (const char* module : dependencies) {
    const bool available = be_getmodule(berry.vm,module);
    be_pop(berry.vm,be_top(berry.vm));
    if (!available) {
      Serial.printf("Tasmota Matter dependency missing: module=%s\n",module);
      return false;
    }
  }
  Serial.println("Tasmota Matter dependencies ready.");
  if (!lh_vm_function("lh_begin")) return false;
  be_pushstring(berry.vm,configuration);
  const bool ok = lh_vm_result(1); be_pop(berry.vm,be_top(berry.vm));
  return ok;
}
extern "C" bool lh_tasmota_report(uint16_t endpoint,bool on,bool reachable) {
  if (!berry.vm) return false;
  if (!lh_vm_function("lh_report")) return false;
  be_pushint(berry.vm,endpoint); be_pushbool(berry.vm,on); be_pushbool(berry.vm,reachable);
  const bool ok = lh_vm_result(3); be_pop(berry.vm,be_top(berry.vm)); return ok;
}
extern "C" bool lh_tasmota_info(char* json,size_t size) {
  if (!berry.vm || !json || !size) return false;
  lh_print_pairing_diag();
  if (!lh_vm_function("lh_info")) return false;
  if (!lh_vm_result(0)) return false;
  const bool ok = be_isstring(berry.vm,-1) && strlen(be_tostring(berry.vm,-1)) < size;
  if (ok) strlcpy(json,be_tostring(berry.vm,-1),size);
  be_pop(berry.vm,be_top(berry.vm)); return ok;
}
extern "C" bool lh_tasmota_commission() {
  if (!berry.vm) return false;
  if (!lh_vm_function("lh_commission")) return false;
  const bool ok = lh_vm_result(0); be_pop(berry.vm,be_top(berry.vm)); return ok;
}

extern "C" bool lh_tasmota_checkpoint() {
  if (!berry.vm || !lh_vm_function("lh_checkpoint")) return false;
  const bool ok = lh_vm_result(0); be_pop(berry.vm,be_top(berry.vm)); return ok;
}
