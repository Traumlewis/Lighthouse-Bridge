#pragma once
#include <app/CommandHandler.h>
#include <app/CommandHandlerInterface.h>
#include <app/CommandHandlerInterfaceRegistry.h>
#include <app/data-model/Decode.h>
#include <app/server/Server.h>
#include <mdns.h>
#include <atomic>
#include "matter_config.h"
#if !defined(CONFIG_IDF_TARGET_ESP32) || !defined(CONFIG_ESP_MATTER_ENABLE_DATA_MODEL) || CONFIG_ENABLE_CHIPOBLE
#error Classic ESP32 Matter with CHIPoBLE disabled is required
#endif
using namespace chip::app::Clusters;
constexpr char kMatterTestName[] = "Lighthouse-Bridge Matter Test";
std::atomic<bool> matterReady{false}, matterPaired{false}, matterServicesReady{false};
std::atomic<bool> matterWindowOpen{false}, matterWindowPending{false}, matterDnsReady{false};
std::atomic<bool> matterHostnameDirty{false}, matterWorkPending{false};
std::atomic<const char*> matterError{"WAITING_WIFI"};
std::atomic<const char*> matterWindowError{""};
std::unique_ptr<MatterRegistry> matterRegistry;
bool matterRegistryValid = false;
String manualCode;

class MatterBridgeInitializer : public ArduinoMatter {
public:
  static void prepare() { _init(); }
};

struct MatterStation : public MatterEndPoint {
  MatterBinding binding;
  bool active = false, reporting = false, reported = false, lastOn = false, lastReachable = false;
  bool attributeChangeCB(uint16_t, uint32_t cluster, uint32_t attribute, esp_matter_attr_val_t*) override {
    // Commands are intercepted separately. No requested value may masquerade
    // as a physical reading through the SDK's normal OnOff update path.
    return cluster != OnOff::Id || attribute != OnOff::Attributes::OnOff::Id || reporting;
  }
};
std::unique_ptr<MatterStation[]> lamps;

int lampIndex(uint16_t endpoint) {
  for (unsigned i = 0; i < kStations; ++i)
    if (lamps[i].active && lamps[i].getEndPointId() == endpoint) return i;
  return -1;
}
int bindingStationIndex(const MatterBinding& binding) { // Caller holds stateMutex.
  const int index = stationHueIndex(configs,binding.hueId);
  return index >= 0 && !strcmp(configs[index].mac,binding.mac) ? index : -1;
}

class MatterPowerHandler : public chip::app::CommandHandlerInterface {
public:
  MatterPowerHandler() : CommandHandlerInterface(chip::NullOptional,OnOff::Id) {}
  void InvokeCommand(HandlerContext& context) override {
    using chip::Protocols::InteractionModel::Status;
    context.SetCommandHandled();
    const auto command = context.mRequestPath.mCommandId;
    auto& reader = context.GetReader();
    bool valid = false;
    if (command == OnOff::Commands::On::Id) {
      OnOff::Commands::On::DecodableType data;
      valid = chip::app::DataModel::Decode(reader,data) == CHIP_NO_ERROR;
    } else if (command == OnOff::Commands::Off::Id) {
      OnOff::Commands::Off::DecodableType data;
      valid = chip::app::DataModel::Decode(reader,data) == CHIP_NO_ERROR;
    } else if (command == OnOff::Commands::Toggle::Id) {
      OnOff::Commands::Toggle::DecodableType data;
      valid = chip::app::DataModel::Decode(reader,data) == CHIP_NO_ERROR;
    }
    bool accepted = false, on = command == OnOff::Commands::On::Id;
    unsigned stationId = 0;
    const int lamp = lampIndex(context.mRequestPath.mEndpointId);
    if (valid && lamp >= 0 && matterReady.load() && matterServicesReady.load()) {
      Lock lock;
      const int index = bindingStationIndex(lamps[lamp].binding);
      if (index >= 0 && !blePaused && !portalRequested && !otaPrepared && !restartPending) {
        if (command == OnOff::Commands::Toggle::Id) {
          valid = freshMeasurement(states[index],millis());
          on = !measuredOn(states[index]);
        }
        if (valid) {
          request(states[index],on ? Target::On : Target::Sleep,millis());
          accepted = true; stationId = configs[index].id;
        }
      }
    }
    context.mCommandHandler.AddStatus(context.mRequestPath,accepted ? Status::Success : Status::Failure);
    Serial.printf("Matter command: station=%u, accepted=%s, target=%s\n",stationId,
      accepted ? "true" : "false",on ? "ON" : "SLEEP");
  }
};
MatterPowerHandler powerHandler;

void reportMeasurements(intptr_t) {
  for (unsigned n = 0; n < kStations; ++n) {
    auto& lamp = lamps[n];
    if (!lamp.active) continue;
    bool on = false, reachable = false;
    unsigned id = 0;
    {
      Lock lock;
      const int index = bindingStationIndex(lamp.binding);
      if (index >= 0) {
        id = configs[index].id; on = measuredOn(states[index]);
        reachable = freshMeasurement(states[index],millis());
      }
    }
    if (lamp.reported && on == lamp.lastOn && reachable == lamp.lastReachable) continue;
    auto onValue = esp_matter_bool(on), reachableValue = esp_matter_bool(reachable);
    lamp.reporting = true;
    const auto onResult = esp_matter::attribute::update(lamp.getEndPointId(),OnOff::Id,OnOff::Attributes::OnOff::Id,&onValue);
    const auto reachableResult = esp_matter::attribute::update(lamp.getEndPointId(),BridgedDeviceBasicInformation::Id,
      BridgedDeviceBasicInformation::Attributes::Reachable::Id,&reachableValue);
    lamp.reporting = false;
    if (onResult == ESP_OK && reachableResult == ESP_OK) {
      lamp.reported = true; lamp.lastOn = on; lamp.lastReachable = reachable;
      Serial.printf("Matter measurement: station=%u, on=%s, reachable=%s\n",id,
        on ? "true" : "false",reachable ? "true" : "false");
    } else Serial.printf("Matter report failed: station=%u, on_code=%d, reachable_code=%d\n",id,onResult,reachableResult);
  }
}

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

bool createLamp(unsigned index, esp_matter::endpoint_t* parent) {
  using namespace esp_matter;
  const auto& config = configs[index];
  const int binding = matterBindingIndex((*matterRegistry),config.hueId,config.mac);
  if (binding < 0) return false;
  auto& lamp = lamps[index]; lamp.binding = (*matterRegistry).entries[binding];
  auto* endpoint = endpoint::resume(node::get(),ENDPOINT_FLAG_NONE,lamp.binding.endpoint,&lamp);
  if (!endpoint) return false;
  endpoint::on_off_light::config_t lightConfig;
  lightConfig.on_off.on_off = false;
  lightConfig.on_off_lighting.start_up_on_off = nullptr;
  endpoint::bridged_node::config_t bridgedConfig;
  bridgedConfig.bridged_device_basic_information.reachable = false;
  if (endpoint::on_off_light::add(endpoint,&lightConfig) != ESP_OK ||
      endpoint::bridged_node::add(endpoint,&bridgedConfig) != ESP_OK ||
      endpoint::set_parent_endpoint(endpoint,parent) != ESP_OK) return false;
  auto* basic = cluster::get(endpoint,BridgedDeviceBasicInformation::Id);
  size_t length = std::min(strlen(config.name),static_cast<size_t>(32));
  while (length && (static_cast<uint8_t>(config.name[length]) & 0xc0) == 0x80) --length;
  String unique = WiFi.macAddress(); unique.replace(":","");
  unique = "lhm-" + unique + "-" + String(config.hueId);
  // Existing survivors keep their old unique ID. A replacement MAC gets a new
  // endpoint; including that endpoint disambiguates replacements after this test.
  if (lamp.binding.endpoint != 1) {
    unique = WiFi.macAddress(); unique.replace(":","");
    unique = "lhm-" + unique + "-e" + String(lamp.binding.endpoint);
  }
  if (!cluster::bridged_device_basic_information::attribute::create_node_label(basic,const_cast<char*>(config.name),length) ||
      !cluster::bridged_device_basic_information::attribute::create_unique_id(basic,const_cast<char*>(unique.c_str()),unique.length())) return false;
  lamp.setEndPointId(lamp.binding.endpoint); lamp.active = true;
  return true;
}

bool matterBridgeBegin() {
  matterRegistry.reset(new(std::nothrow) MatterRegistry);
  lamps.reset(new(std::nothrow) MatterStation[kStations]);
  if (!matterRegistry || !lamps) { matterError = "OUT_OF_MEMORY"; return false; }
  using namespace esp_matter;
  Matter.selectNetwork(MATTER_NETWORK_WIFI,true);
  Matter.setVendorName("Lighthouse Bridge");
  Matter.setProductName(kMatterTestName); Matter.setDeviceName(kMatterTestName);
  Preferences own;
  if (!own.begin("lhmatter",false)) { matterError = "MATTER_STORAGE_FAILED"; return false; }
  uint32_t passcode = own.getUInt("passcode",0);
  const uint16_t discriminator = own.getUShort("discriminator",esp_random() & 0xfff);
  if (!own.isKey("discriminator") && own.putUShort("discriminator",discriminator) != sizeof(discriminator)) {
    own.end(); matterError = "MATTER_STORAGE_FAILED"; return false;
  }
  if (!passcode || !Matter.setSetupPasscode(passcode)) {
    do { passcode = 1 + esp_random() % 99999998; } while (!Matter.setSetupPasscode(passcode));
    if (own.putUInt("passcode",passcode) != sizeof(passcode) ||
        own.putUShort("discriminator",discriminator) != sizeof(discriminator)) {
      own.end(); matterError = "MATTER_STORAGE_FAILED"; return false;
    }
  }
  if (!loadRegistry(own)) { own.end(); return false; }
  own.end(); Matter.setSetupDiscriminator(discriminator);
  MatterBridgeInitializer::prepare();
  auto* parent = endpoint::resume(node::get(),ENDPOINT_FLAG_NONE,kMatterAggregatorEndpoint,nullptr);
  endpoint::aggregator::config_t aggregate;
  if (!parent || endpoint::aggregator::add(parent,&aggregate) != ESP_OK) { matterError = "MATTER_BRIDGE_FAILED"; return false; }
  for (unsigned i = 0; i < kStations; ++i) if (configs[i].id && configs[i].mac[0] && !createLamp(i,parent)) {
    matterError = "MATTER_ENDPOINT_FAILED"; return false;
  }
  if (chip::app::CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(&powerHandler) != CHIP_NO_ERROR) {
    matterError = "MATTER_COMMAND_FAILED"; return false;
  }
  Matter.onEvent([](matterEvent_t event, const chip::DeviceLayer::ChipDeviceEvent*) {
    if (event == MATTER_DNSSD_INITIALIZED) { matterDnsReady = true; matterHostnameDirty = true; }
    matterPaired = Matter.isDeviceCommissioned();
    matterWindowOpen = chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen();
  });
  Matter.begin(); manualCode = Matter.getManualPairingCode();
  if (!manualCode.length()) { matterError = "MATTER_START_FAILED"; return false; }
  for (unsigned n = 0; n < kStations; ++n) if (lamps[n].active &&
      chip::app::CommandHandlerInterfaceRegistry::Instance().GetCommandHandler(lamps[n].getEndPointId(),OnOff::Id) != &powerHandler) {
    matterError = "MATTER_HANDLER_CHANGED"; return false;
  }
  matterReady = true; matterError = "";
  return true;
}

// Save the mapping checkpoint before the station record. Surviving identities
// remain valid even if the subsequent station save is interrupted.
bool matterPrepareConfiguration(const StationConfig* next, std::string& error) {
  if (!matterRegistryValid || restartPending) { error = "MATTER_NOT_READY"; return false; }
  std::unique_ptr<MatterRegistry> planned(new(std::nothrow) MatterRegistry);
  if (!planned) { error = "OUT_OF_MEMORY"; return false; }
  if (!planMatterRegistry(configs,next,(*matterRegistry),*planned,error)) return false;
  DynamicJsonDocument doc(kMatterRegistryJsonCapacity);
  writeMatterRegistry(doc.to<JsonObject>(),*planned);
  if (doc.overflowed()) { error = "OUT_OF_MEMORY"; return false; }
  String json; serializeJson(doc,json);
  Preferences own;
  if (!own.begin("lhmatter",false)) { error = "MATTER_STORAGE_FAILED"; return false; }
  const bool saved = own.putString("endpoints",json) == json.length(); own.end();
  if (!saved) { error = "MATTER_STORAGE_FAILED"; return false; }
  (*matterRegistry) = *planned;
  return true;
}
void matterConfigurationChanged() {
  Lock lock;
  matterReady = false; matterError = "RESTARTING";
  blePaused = true; restartAt = millis(); restartPending = true;
}
void matterPrivateInfo(JsonDocument& doc, bool includeCode) {
  doc["protocol"] = 1; doc["version"] = LH_FIRMWARE_VERSION;
  doc["bridge_name"] = kMatterTestName;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["ready"] = matterReady.load(); doc["paired"] = matterPaired.load();
  doc["commissioning_open"] = matterWindowOpen.load();
  doc["commissioning_pending"] = matterWindowPending.load();
  doc["commissioning_error"] = matterWindowError.load(); doc["error"] = matterError.load();
  if (includeCode && matterReady && manualCode.length()) doc["pairing_code"] = manualCode;
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
void matterCommissioningWork(intptr_t) {
  auto& manager = chip::Server::GetInstance().GetCommissioningWindowManager();
  const auto result = manager.IsCommissioningWindowOpen() ? CHIP_NO_ERROR :
    manager.OpenBasicCommissioningWindow(chip::System::Clock::Seconds32(900),chip::CommissioningWindowAdvertisement::kDnssdOnly);
  matterWindowOpen = manager.IsCommissioningWindowOpen();
  matterWindowError = result == CHIP_NO_ERROR ? "" : "MATTER_WINDOW_FAILED";
  matterWindowPending = false;
}
bool matterOpenCommissioning() {
  if (!matterReady || restartPending) return false;
  if (matterWindowOpen || matterWindowPending.exchange(true)) return true;
  matterWindowError = "";
  if (chip::DeviceLayer::PlatformMgr().ScheduleWork(matterCommissioningWork,0) != CHIP_NO_ERROR) {
    matterWindowPending = false; matterWindowError = "MATTER_WINDOW_FAILED"; return false;
  }
  return true;
}
char matterHostname[33]{};
void matterSetHostname(const String& value) {
  Lock lock; strlcpy(matterHostname,value.c_str(),sizeof(matterHostname)); matterHostnameDirty = true;
}
void matterPeriodicWork(intptr_t) {
  if (matterReady) reportMeasurements(0);
  matterPaired = Matter.isDeviceCommissioned();
  matterWindowOpen = chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen();
  if (matterDnsReady && matterHostnameDirty.exchange(false)) {
    char value[33]; { Lock lock; strlcpy(value,matterHostname,sizeof(value)); }
    // Matter owns mDNS. Never tear down its DNS-SD services with MDNS.end().
    const auto result = mdns_hostname_set(value);
    const auto service = mdns_service_exists("_http","_tcp",nullptr) ? ESP_OK :
      mdns_service_add(nullptr,"_http","_tcp",80,nullptr,0);
    if (result != ESP_OK || service != ESP_OK) matterHostnameDirty = true;
  }
  matterWorkPending = false;
}
void matterBridgeLoop() {
  static uint32_t last = 0;
  if (matterReady && millis() - last >= 500 && !matterWorkPending.exchange(true)) {
    last = millis();
    if (chip::DeviceLayer::PlatformMgr().ScheduleWork(matterPeriodicWork,0) != CHIP_NO_ERROR) matterWorkPending = false;
  }
}
