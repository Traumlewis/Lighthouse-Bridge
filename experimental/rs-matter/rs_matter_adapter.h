#pragma once
#include "rs_runtime.h"
#include "matter_config.h"
#include "rs_matter_commands.h"
#include <mdns.h>
void processSerial();
constexpr char kMatterTestName[] = "Lighthouse-Bridge Matter Test";
std::atomic<bool> matterInitializing{false}, matterReady{false}, matterPaired{false};
std::atomic<bool> matterServicesReady{false}, matterWindowOpen{false}, matterWindowPending{false};
std::atomic<const char*> matterError{"WAITING_WIFI"}, matterStartupPhase{"waiting_wifi"}, matterWindowError{""};
std::unique_ptr<MatterRegistry> matterRegistry;
bool matterRegistryValid = false, rsStarted = false;
String manualCode;
bool windowDispatched=false;
struct MatterBridgeInitializer { static bool started() { return rsStarted; } };
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
    result = acceptRsMatterCommand(configs,states,*matterRegistry,endpoint,command,millis(),
      matterReady && bleWorkerAvailable,blePaused || portalRequested || otaPrepared || restartPending);
  }
  Serial.printf("Matter command: station=%u, accepted=%s, target=%s\n",result.station,
    result.station ? "true":"false",result.on ? "ON":"SLEEP");
  return result.station != 0;
}

bool matterRefreshInfo() {
  char code[12]{}; bool paired=false,open=false;
  if(!lh_rs_info(code,sizeof(code),&paired,&open)) return false;
  manualCode=code; matterPaired=paired; matterWindowOpen=open;
  return !manualCode.length() || manualCode.length()==11;
}
bool matterBridgeBegin() {
  matterStartupPhase="rs_storage";
  matterRegistry.reset(new(std::nothrow) MatterRegistry);
  if(!matterRegistry) {matterError="OUT_OF_MEMORY";return false;}
  Preferences own;
  if(!own.begin("lhmatter",false)) {matterError="MATTER_STORAGE_FAILED";return false;}
  uint32_t passcode=own.getUInt("passcode",0);
  const uint16_t discriminator=own.getUShort("discriminator",esp_random()&0xfff);
  if(!passcode) {
    do {passcode=1+esp_random()%99999998;} while(passcode%11111111==0 || passcode==12345678 || passcode==87654321);
    if(own.putUInt("passcode",passcode)!=sizeof(passcode)){matterError="MATTER_STORAGE_FAILED";return false;}
  }
  if((!own.isKey("discriminator") && own.putUShort("discriminator",discriminator)!=sizeof(discriminator)) || !loadRegistry(own)){matterError="MATTER_STORAGE_FAILED";return false;}
  own.end();
  std::unique_ptr<RsConfig> config(new(std::nothrow) RsConfig{});
  if(!config){matterError="OUT_OF_MEMORY";return false;}
  config->passcode=passcode;config->discriminator=discriminator;
  String bridgeMac=WiFi.macAddress();bridgeMac.replace(":","");
  String identity="lhm-"+bridgeMac;
  strncpy(config->serial,identity.c_str(),sizeof(config->serial)-1);
  for(unsigned i=0;i<kStations;++i){
    if(!configs[i].id || !configs[i].mac[0]) continue;
    const int binding=matterBindingIndex(*matterRegistry,configs[i].hueId,configs[i].mac);
    if(binding<0){matterError="MATTER_REGISTRY_INVALID";return false;}
    auto& station=config->stations[config->count++];
    station.endpoint=matterRegistry->entries[binding].endpoint;
    memcpy(station.name,configs[i].name,matterLabelBytes(configs[i].name));
    const String unique="lhm-"+bridgeMac+(station.endpoint==1 ? "-"+String(configs[i].hueId) : "-e"+String(station.endpoint));
    strncpy(station.unique,unique.c_str(),sizeof(station.unique)-1);
  }
  matterStartupPhase="rs_model";
  Serial.printf("rs-matter allocations: stack_bytes=%u task_bytes=%u free_8bit=%u largest_8bit=%u\n",
    static_cast<unsigned>(lh_rs_stack_bytes()),static_cast<unsigned>(lh_rs_task_bytes()),matterDataFree(),matterDataLargest());
  rsStarted=lh_rs_begin(config.get());config.reset();
  if(!rsStarted){matterError="MATTER_START_FAILED";return false;}
  const uint32_t began=millis();
  int state=0;
  while((state=lh_rs_pump())==0 && millis()-began<15000){server.handleClient();processSerial();delay(2);}
  if(state!=1 || !matterRefreshInfo()){rsStarted=false;matterError="MATTER_START_FAILED";return false;}
  matterReady=true;matterError="";matterStartupPhase="ready";
  Serial.println("rs-matter endpoints ready; no startup power commands sent.");
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
  doc["engine"] = "rs-matter";
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
  windowDispatched=false; manualCode = ""; matterWindowError = ""; return true;
}
void matterSetHostname(const String& value) {
  // rs-matter owns its DNS-SD responder. Never stop it through Arduino mDNS.
  (void)value; // The responder reloads the saved hostname on its next check.
}
extern "C" uint32_t lh_rs_observed(uint16_t endpoint) {
  Lock lock;
  for(unsigned i=0;i<kStations;++i){
    if(!configs[i].id || !configs[i].mac[0]) continue;
    const int binding=matterBindingIndex(*matterRegistry,configs[i].hueId,configs[i].mac);
    if(binding>=0 && matterRegistry->entries[binding].endpoint==endpoint)
      return (measuredOn(states[i])?1:0) | (freshMeasurement(states[i],millis())?2:0);
  }
  return 0;
}
void matterBridgeLoop() {
  if(!rsStarted) return;
  if(lh_rs_pump()<0){matterReady=false;matterError="MATTER_RUNTIME_FAILED";return;}
  static uint32_t last=0;
  if(!matterReady || millis()-last<250) return;
  last=millis();
  static uint32_t logged=0;
  if(millis()-logged>=15000){
    logged=millis();
    Serial.printf("rs-matter diag: ready=%s paired=%s window=%s free_8bit=%u largest_8bit=%u min_8bit=%u stack_watermark=%u\n",
      matterReady.load()?"true":"false",matterPaired.load()?"true":"false",matterWindowOpen.load()?"open":"closed",
      matterDataFree(),matterDataLargest(),heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
  }
  if(matterWindowPending){
    if(!windowDispatched){
      windowDispatched=lh_rs_open();
      if(!windowDispatched){matterWindowError="MATTER_WINDOW_FAILED";matterWindowPending=false;}
    }else if(!lh_rs_pending()) matterWindowPending=false;
  }
  if(!matterRefreshInfo()) matterError="MATTER_STATUS_FAILED";
}

extern "C" void lh_rs_hostname(char* out,size_t size){if(size){strncpy(out,hostname.c_str(),size-1);out[size-1]=0;}}

extern "C" uint32_t lh_rs_generation(uint16_t endpoint){
  Lock lock;
  for(unsigned i=0;i<kStations;++i){
    if(!configs[i].id || !configs[i].mac[0]) continue;
    const int binding=matterBindingIndex(*matterRegistry,configs[i].hueId,configs[i].mac);
    if(binding>=0 && matterRegistry->entries[binding].endpoint==endpoint) return states[i].generation;
  }
  return 0;
}
