// Isolated multi-station Matter test; configure Wi-Fi/stations with stable first.
#include <Matter.h>
#include <WiFi.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include <esp_wifi.h>
#include <app/CommandHandler.h>
#include <app/CommandHandlerInterface.h>
#include <app/CommandHandlerInterfaceRegistry.h>
#include <app/data-model/Decode.h>
#include <atomic>
#include "matter_config.h"

#if !defined(CONFIG_IDF_TARGET_ESP32) || !defined(CONFIG_ESP_MATTER_ENABLE_DATA_MODEL)
#error Classic ESP32 with Matter is required
#endif
#if CONFIG_ENABLE_CHIPOBLE
#error CHIPoBLE must be disabled; NimBLE belongs to the Lighthouse worker
#endif

using namespace bridge;
using namespace chip::app::Clusters;
constexpr char kVersion[] = "0.2.0-matter";
constexpr char kMatterTestName[] = "Lighthouse-Bridge Matter Test";
SemaphoreHandle_t stateMutex;
StationConfig configs[kStations];
StationState states[kStations];
bool refreshRequested[kStations]{};
bool blePaused = false, bleActive = false, scanPending = false, scanning = false;
uint32_t scanFinishedMs = 0;
struct FoundStation { char name[48]; char mac[18]; uint8_t type; int rssi; };
FoundStation found[16]{};
unsigned foundCount = 0;
bool foundTruncated = false;
bool matterStarted = false, configLoaded = false;
std::atomic<bool> workerStarted{false};
const char* setupError = "WAITING_WIFI";
String manualCode, serialInput;
MatterRegistry registry;

struct Lock {
  Lock() { xSemaphoreTake(stateMutex, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(stateMutex); }
};

// Generated from src/main.cpp by build_matter.py. Both builds use the exact
// production worker, including retry limits, cleanup and read-only confirmation.
#include "shared_ble_worker.h"

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
MatterStation lamps[kStations];

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
    if (valid && lamp >= 0 && workerStarted.load()) {
      Lock lock;
      const int index = bindingStationIndex(lamps[lamp].binding);
      if (index >= 0 && !blePaused) {
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
  for (auto& lamp : lamps) {
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

bool loadConfiguration() {
  Preferences saved;
  if (!saved.begin("lhbridge",true)) return false;
  const String json = saved.getString("stations",""); saved.end();
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc,json)) return false;
  const bool legacy = doc.is<JsonArray>();
  uint32_t counter = legacy ? 6 : doc["next_id"].as<uint32_t>();
  std::string error;
  return parseStationList(legacy ? doc.as<JsonVariantConst>() : doc["stations"].as<JsonVariantConst>(),
    configs,nullptr,counter,true,legacy,error);
}

bool loadRegistry(Preferences& own) {
  const String saved = own.getString("endpoints","");
  std::string error;
  if (saved.length()) {
    DynamicJsonDocument doc(8192);
    MatterRegistry checkpoint;
    if (deserializeJson(doc,saved) || !parseMatterRegistry(doc.as<JsonVariantConst>(),checkpoint,error)) {
      setupError = "MATTER_REGISTRY_INVALID"; return false;
    }
    if (!planMatterRegistry(configs,configs,checkpoint,registry,error)) {
      setupError = "MATTER_IDS_EXHAUSTED"; return false;
    }
  } else if (!seedMatterRegistry(configs,registry,error)) { setupError = "MATTER_IDS_EXHAUSTED"; return false; }
  DynamicJsonDocument doc(8192);
  writeMatterRegistry(doc.to<JsonObject>(),registry);
  String json; serializeJson(doc,json);
  if (saved != json && own.putString("endpoints",json) != json.length()) { setupError = "MATTER_STORAGE_FAILED"; return false; }
  return true;
}

bool createLamp(unsigned index, esp_matter::endpoint_t* parent) {
  using namespace esp_matter;
  const auto& config = configs[index];
  const int binding = matterBindingIndex(registry,config.hueId,config.mac);
  if (binding < 0) return false;
  auto& lamp = lamps[index]; lamp.binding = registry.entries[binding];
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

bool beginMatter() {
  using namespace esp_matter;
  Matter.selectNetwork(MATTER_NETWORK_WIFI,true);
  Matter.setVendorName("Lighthouse Bridge");
  Matter.setProductName(kMatterTestName); Matter.setDeviceName(kMatterTestName);
  Preferences own;
  if (!own.begin("lhmatter",false)) { setupError = "MATTER_STORAGE_FAILED"; return false; }
  uint32_t passcode = own.getUInt("passcode",0);
  const uint16_t discriminator = own.getUShort("discriminator",esp_random() & 0xfff);
  if (!passcode || !Matter.setSetupPasscode(passcode)) {
    do { passcode = 1 + esp_random() % 99999998; } while (!Matter.setSetupPasscode(passcode));
    if (own.putUInt("passcode",passcode) != sizeof(passcode) ||
        own.putUShort("discriminator",discriminator) != sizeof(discriminator)) {
      own.end(); setupError = "MATTER_STORAGE_FAILED"; return false;
    }
  }
  if (!loadRegistry(own)) { own.end(); return false; }
  own.end(); Matter.setSetupDiscriminator(discriminator);
  MatterBridgeInitializer::prepare();
  auto* parent = endpoint::resume(node::get(),ENDPOINT_FLAG_NONE,kMatterAggregatorEndpoint,nullptr);
  endpoint::aggregator::config_t aggregate;
  if (!parent || endpoint::aggregator::add(parent,&aggregate) != ESP_OK) { setupError = "MATTER_BRIDGE_FAILED"; return false; }
  for (unsigned i = 0; i < kStations; ++i) if (configs[i].id && configs[i].mac[0] && !createLamp(i,parent)) {
    setupError = "MATTER_ENDPOINT_FAILED"; return false;
  }
  if (chip::app::CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(&powerHandler) != CHIP_NO_ERROR) {
    setupError = "MATTER_COMMAND_FAILED"; return false;
  }
  Matter.begin(); matterStarted = true; manualCode = Matter.getManualPairingCode();
  if (!manualCode.length()) { setupError = "MATTER_START_FAILED"; return false; }
  for (auto& lamp : lamps) if (lamp.active &&
      chip::app::CommandHandlerInterfaceRegistry::Instance().GetCommandHandler(lamp.getEndPointId(),OnOff::Id) != &powerHandler) {
    setupError = "MATTER_HANDLER_CHANGED"; return false;
  }
  if (!NimBLEDevice::init("Lighthouse Matter")) { setupError = "BLE_INIT_FAILED"; return false; }
  if (xTaskCreatePinnedToCore(bleWorker,"lighthouse-ble",8192,nullptr,1,nullptr,0) != pdPASS) {
    setupError = "BLE_WORKER_FAILED"; return false;
  }
  workerStarted = true; setupError = "";
  Serial.printf("Matter prototype ready: stations=%u; no power command sent on startup.\n",registry.count);
  return true;
}

void writeMeasurement(JsonObject item, unsigned index, uint32_t now) { // Caller holds stateMutex.
  item["id"] = configs[index].id; item["name"] = configs[index].name;
  item["reachable"] = freshMeasurement(states[index],now);
  item["observed_raw"] = states[index].observed; item["observed_ms"] = states[index].observedMs;
  item["command_status"] = commandStatus(states[index],now);
}

void emitInfo() {
  DynamicJsonDocument doc(8192);
  doc["protocol"] = 1; doc["version"] = kVersion; doc["bridge_name"] = kMatterTestName;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["ready"] = workerStarted.load(); doc["paired"] = matterStarted && Matter.isDeviceCommissioned();
  if (workerStarted && !doc["paired"].as<bool>()) doc["pairing_code"] = manualCode;
  else doc["pairing_code"] = nullptr;
  doc["error"] = setupError; doc["station_id"] = 0; doc["station_name"] = "";
  doc["reachable"] = false; doc["observed_raw"] = -1;
  auto list = doc.createNestedArray("stations");
  {
    Lock lock; const uint32_t now = millis(); doc["uptime_ms"] = now;
    for (unsigned i = 0; i < kStations; ++i) {
      if (!configs[i].id || !configs[i].mac[0]) continue;
      writeMeasurement(list.createNestedObject(),i,now);
      if (!doc["station_id"].as<unsigned>()) {
        doc["station_id"] = configs[i].id; doc["station_name"] = configs[i].name;
        doc["reachable"] = freshMeasurement(states[i],now); doc["observed_raw"] = states[i].observed;
        doc["observed_ms"] = states[i].observedMs; doc["command_status"] = commandStatus(states[i],now);
      }
    }
  }
  Serial.print("LH_MATTER_INFO "); serializeJson(doc,Serial); Serial.println();
}

void setup() {
  Serial.begin(115200); stateMutex = xSemaphoreCreateMutex();
  if (!stateMutex) abort();
  configLoaded = loadConfiguration();
  if (!configLoaded) { setupError = "CONFIGURE_STABLE_FIRST"; return; }
  WiFi.setSleep(true); WiFi.mode(WIFI_STA); WiFi.enableIPv6(); WiFi.begin();
  Serial.printf("Lighthouse Matter prototype %s; waiting for saved Wi-Fi.\n",kVersion);
}

void loop() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n') { serialInput.trim(); if (serialInput == "LH_MATTER_INFO") emitInfo(); serialInput = ""; }
    else if (serialInput.length() < 64) serialInput += c;
    else serialInput = "";
  }
  static bool wifiWasConnected = false, attempted = false;
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wifiWasConnected && esp_wifi_set_ps(WIFI_PS_MIN_MODEM) != ESP_OK) {
    setupError = "WIFI_SLEEP_FAILED"; attempted = true;
  }
  wifiWasConnected = connected;
  if (connected && configLoaded && !attempted) { attempted = true; beginMatter(); }
  static uint32_t lastReport = 0;
  if (workerStarted && millis() - lastReport >= 500) {
    lastReport = millis(); chip::DeviceLayer::PlatformMgr().ScheduleWork(reportMeasurements,0);
  }
  delay(10);
}
