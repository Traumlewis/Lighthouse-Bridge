#pragma once
#include "tasmota_runtime.h"
#include "matter_config.h"
#include "tasmota_commands.h"
#include <mdns.h>
constexpr char kMatterTestName[] = "Lighthouse-Bridge Matter Test";
std::atomic<bool> matterInitializing{false}, matterReady{false}, matterPaired{false};
std::atomic<bool> matterServicesReady{false}, matterWindowOpen{false}, matterWindowPending{false};
std::atomic<const char*> matterError{"WAITING_WIFI"}, matterStartupPhase{"waiting_wifi"}, matterWindowError{""};
std::unique_ptr<MatterRegistry> matterRegistry;
bool matterRegistryValid = false, tasmotaStarted = false;
String manualCode;
struct MatterBridgeInitializer { static bool started() { return tasmotaStarted; } };
bool matterBeginResources() { return true; }
void matterEndResources() {}
bool loadRegistry(Preferences& own) {
  const String saved = own.getString("endpoints","");
  if (own.isKey("endpoints") && !saved.length()) { matterError = "MATTER_STORAGE_FAILED"; return false; }
  std::string error;
  if (saved.length()) {
    DynamicJsonDocument doc(kMatterRegistryJsonCapacity);
    MatterRegistry checkpoint;
    if (deserializeJson(doc,saved) || !parseMatterRegistry(doc.as<JsonVariantConst>(),checkpoint,error)) {
      matterError = "MATTER_REGISTRY_INVALID"; return false;
    }
    if (!planMatterRegistry(configs,configs,checkpoint,(*matterRegistry),error)) {
      matterError = "MATTER_IDS_EXHAUSTED"; return false;
    }
  } else if (!seedMatterRegistry(configs,(*matterRegistry),error)) { matterError = "MATTER_IDS_EXHAUSTED"; return false; }
  DynamicJsonDocument doc(kMatterRegistryJsonCapacity);
  writeMatterRegistry(doc.to<JsonObject>(),(*matterRegistry));
  if (doc.overflowed()) { matterError = "OUT_OF_MEMORY"; return false; }
  String json; serializeJson(doc,json);
  if (saved != json && own.putString("endpoints",json) != json.length()) { matterError = "MATTER_STORAGE_FAILED"; return false; }
  matterRegistryValid = true;
  return true;
}


extern "C" bool lh_queue_power(uint16_t endpoint,unsigned command) {
  if (!matterRegistry) return false;
  MatterCommandAcceptance result;
  {
    Lock lock;
    result = acceptTasmotaCommand(configs,states,*matterRegistry,endpoint,command,millis(),
      matterReady && bleWorkerAvailable,blePaused || portalRequested || otaPrepared || restartPending);
  }
  Serial.printf("Matter command: station=%u, accepted=%s, target=%s\n",result.station,
    result.station ? "true":"false",result.on ? "ON":"SLEEP");
  return result.station != 0;
}

bool matterRefreshInfo() {
  char body[128];
  if (!lh_tasmota_info(body,sizeof(body))) return false;
  StaticJsonDocument<256> info;
  if (deserializeJson(info,body) || (!info["code"].isNull() && !info["code"].is<const char*>())) return false;
  manualCode = info["code"].isNull() ? "" : info["code"].as<const char*>();
  matterPaired = info["paired"].as<bool>(); matterWindowOpen = info["open"].as<bool>();
  return !manualCode.length() || manualCode.length() == 11;
}
bool matterBridgeBegin() {
  matterStartupPhase = "tasmota_storage";
  if (!lh_tasmota_fs_ready()) { matterError = "MATTER_STORAGE_FAILED"; return false; }
  matterRegistry.reset(new(std::nothrow) MatterRegistry);
  if (!matterRegistry) { matterError = "OUT_OF_MEMORY"; return false; }
  Preferences own;
  if (!own.begin("lhmatter",false)) { matterError = "MATTER_STORAGE_FAILED"; return false; }
  uint32_t passcode = own.getUInt("passcode",0);
  const uint16_t discriminator = own.getUShort("discriminator",esp_random() & 0xfff);
  if (!passcode) {
    // Matter disallows repeated digits and two sequential demonstration codes.
    do { passcode = 1 + esp_random() % 99999998; }
    while (passcode % 11111111 == 0 || passcode == 12345678 || passcode == 87654321);
    if (own.putUInt("passcode",passcode) != sizeof(passcode)) { own.end(); matterError = "MATTER_STORAGE_FAILED"; return false; }
  }
  if ((!own.isKey("discriminator") && own.putUShort("discriminator",discriminator) != sizeof(discriminator)) || !loadRegistry(own)) {
    own.end(); matterError = "MATTER_STORAGE_FAILED"; return false;
  }
  own.end();
  DynamicJsonDocument doc(kStationJsonCapacity);
  doc["passcode"] = passcode; doc["discriminator"] = discriminator;
  doc["next_endpoint"] = matterRegistry->nextEndpoint;
  JsonObject stations = doc.createNestedObject("stations");
  String bridgeMac = WiFi.macAddress(); bridgeMac.replace(":","");
  for (unsigned i = 0; i < kStations; ++i) {
    if (!configs[i].id || !configs[i].mac[0]) continue;
    const int binding = matterBindingIndex(*matterRegistry,configs[i].hueId,configs[i].mac);
    if (binding < 0) { matterError = "MATTER_REGISTRY_INVALID"; return false; }
    const uint16_t endpoint = matterRegistry->entries[binding].endpoint;
    auto station = stations.createNestedObject(String(endpoint));
    station["type"] = "lighthouse";
    String label(configs[i].name);
    const size_t length = matterLabelBytes(configs[i].name);
    label.remove(length); station["name"] = label;
    station["unique"] = "lhm-" + bridgeMac + (endpoint == 1 ? "-" + String(configs[i].hueId) : "-e" + String(endpoint));
  }
  if (doc.overflowed()) { matterError = "OUT_OF_MEMORY"; return false; }
  String json; serializeJson(doc,json);
  matterStartupPhase = "tasmota_model";
  tasmotaStarted = lh_tasmota_begin(json.c_str());
  json = String(); doc.clear();
  if (!tasmotaStarted || !matterRefreshInfo()) { matterError = "MATTER_START_FAILED"; return false; }
  matterReady = true; matterError = ""; matterStartupPhase = "ready";
  Serial.println("Tasmota Matter endpoints ready; no startup power commands sent.");
  return true;
}
bool matterPrepareConfiguration(const StationConfig* next, std::string& error) {
  if (!matterRegistryValid || matterInitializing || restartPending) { error = "MATTER_NOT_READY"; return false; }
  MatterRegistry planned;
  if (!planMatterRegistry(configs,next,(*matterRegistry),planned,error)) return false;
  String json;
  if (!makeConfigString(json,[&](auto& writer) { return streamRegistry(writer,planned); })) {
    error = "OUT_OF_MEMORY"; return false;
  }
  Preferences own;
  if (!own.begin("lhmatter",false)) { error = "MATTER_STORAGE_FAILED"; return false; }
  const bool saved = own.putString("endpoints",json) == json.length(); own.end();
  if (!saved) { error = "MATTER_STORAGE_FAILED"; return false; }
  (*matterRegistry) = planned;
  return true;
}
void matterConfigurationChanged() {
  Lock lock;
  matterReady = false; matterError = "RESTARTING";
  blePaused = true; restartAt = millis(); restartPending = true;
}
void matterPrivateInfo(JsonDocument& doc, bool includeCode) {
  doc["protocol"] = 1; doc["version"] = LH_FIRMWARE_VERSION;
  doc["engine"] = "tasmota-berry"; doc["engine_heap_bytes"] = lh_tasmota_vm_heap();
  doc["bridge_name"] = kMatterTestName;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["ready"] = matterReady.load() && bleWorkerAvailable.load(); doc["paired"] = matterPaired.load();
  doc["commissioning_open"] = matterWindowOpen.load();
  doc["commissioning_pending"] = matterWindowPending.load();
  doc["startup_phase"] = matterStartupPhase.load();
  doc["commissioning_error"] = matterWindowError.load(); doc["error"] = bleWorkerAvailable.load() ? matterError.load() : bleStartupError.load();
  if (includeCode && !*matterWindowError.load() && !matterWindowPending && matterWindowOpen && matterReady && bleWorkerAvailable && manualCode.length()) doc["pairing_code"] = manualCode;
  else doc["pairing_code"] = nullptr;
}
void emitMatterInfo() {
  DynamicJsonDocument doc(JSON_OBJECT_SIZE(24) + JSON_ARRAY_SIZE(kStations) +
    kStations * (JSON_OBJECT_SIZE(6) + 41) + 256);
  matterPrivateInfo(doc,!matterPaired.load());
  doc["station_id"] = 0; doc["station_name"] = "";
  doc["reachable"] = false; doc["observed_raw"] = -1;
  auto list = doc.createNestedArray("stations");
  if (stateMutex) {
    Lock lock; const uint32_t now = millis(); doc["uptime_ms"] = now;
    for (unsigned i = 0; i < kStations; ++i) {
      if (!configs[i].id || !configs[i].mac[0]) continue;
      auto item = list.createNestedObject();
      item["id"] = configs[i].id; item["name"] = configs[i].name;
      item["reachable"] = freshMeasurement(states[i],now);
      item["observed_raw"] = states[i].observed; item["observed_ms"] = states[i].observedMs;
      item["command_status"] = commandStatus(states[i],now);
      if (!doc["station_id"].as<unsigned>()) {
        doc["station_id"] = configs[i].id; doc["station_name"] = configs[i].name;
        doc["reachable"] = item["reachable"]; doc["observed_raw"] = item["observed_raw"];
        doc["observed_ms"] = states[i].observedMs; doc["command_status"] = item["command_status"];
      }
    }
  } else doc["uptime_ms"] = millis();
  if (doc.overflowed()) return;
  Serial.print("LH_MATTER_INFO "); serializeJson(doc,Serial); Serial.println();
}

bool matterOpenCommissioning() {
  if (!matterReady || !bleWorkerAvailable || restartPending) return false;
  if (matterWindowPending.exchange(true)) return true;
  manualCode = ""; matterWindowError = ""; return true;
}
void matterSetHostname(const String& value) {
  // Add HTTP without stopping the engine's active DNS-SD service.
  if (!tasmotaStarted) return;
  mdns_hostname_set(value.c_str());
  if (!mdns_service_exists("_http","_tcp",nullptr)) mdns_service_add(nullptr,"_http","_tcp",80,nullptr,0);
}
void matterBridgeLoop() {
  static uint32_t last = 0;
  if (!matterReady || millis()-last < 500) return;
  last = millis();
  if (matterWindowPending.exchange(false) && !lh_tasmota_commission()) matterWindowError = "MATTER_WINDOW_FAILED";
  if (!matterRefreshInfo()) { matterError = "MATTER_STATUS_FAILED"; return; }
  for (unsigned i = 0; i < kStations; ++i) {
    bool on,reachable; uint16_t endpoint;
    {
      Lock lock;
      if (!configs[i].id || !configs[i].mac[0]) continue;
      const int binding = matterBindingIndex(*matterRegistry,configs[i].hueId,configs[i].mac);
      if (binding < 0) continue;
      endpoint = matterRegistry->entries[binding].endpoint;
      on = measuredOn(states[i]); reachable = freshMeasurement(states[i],millis());
    }
    if (!lh_tasmota_report(endpoint,on,reachable)) matterError = "MATTER_REPORT_FAILED";
  }
}
