// Experimental single-station Matter bridge for the original ESP32 only.
// Configure Wi-Fi and stations with the stable firmware before installing.
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
#include "station_config.h"

#if !defined(CONFIG_IDF_TARGET_ESP32) || !defined(CONFIG_ESP_MATTER_ENABLE_DATA_MODEL)
#error This prototype requires classic ESP32 with Matter support
#endif
#if CONFIG_ENABLE_CHIPOBLE
#error Matter commissioning must be on-network; NimBLE belongs to the Lighthouse worker
#endif

using namespace bridge;
using namespace chip::app::Clusters;
constexpr char kVersion[] = "0.1.1-matter";
SemaphoreHandle_t stateMutex;
StationConfig station;
StationState measured;
MatterOnOffLight accessory;
bool matterStarted = false, stationLoaded = false;
std::atomic<bool> workerStarted{false};
std::atomic<bool> reportingMeasurement{false};
const char* setupError = "WAITING_WIFI";
uint32_t setupPasscode = 0;
uint16_t setupDiscriminator = 0;
String manualCode, serialInput;
struct Lock {
  Lock() { xSemaphoreTake(stateMutex, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(stateMutex); }
};

void waitForCleanup() {
  const uint32_t started = millis();
  bool warned = false;
  while (NimBLEDevice::getCreatedClientCount()) {
    if (!warned && millis() - started >= 5000) {
      Serial.println("BLE cleanup pending; worker waits."); warned = true;
    }
    delay(20);
  }
}

void recoverAddress() {
  waitForCleanup();
  auto* scan = NimBLEDevice::getScan();
  scan->setActiveScan(false); scan->setInterval(97); scan->setWindow(67);
  auto results = scan->getResults(2000, false);
  bool matched = false;
  for (int i = 0; i < results.getCount(); ++i) {
    const auto* device = results.getDevice(i);
    char mac[18]{}; normalizeMac(device->getAddress().toString().c_str(), mac);
    if (strcmp(mac, station.mac)) continue;
    matched = true;
    static_assert(BLE_HCI_ADV_RPT_EVTYPE_ADV_IND == 0 && BLE_HCI_ADV_RPT_EVTYPE_DIR_IND == 1,
                  "Legacy advertisement enum changed");
    Serial.printf("Station %u: recovery RSSI=%d, address_type=1, advertised_type=%u, adv_type=%u, connectable=%s\n",
      station.id, device->getRSSI(), device->getAddress().getType(), device->getAdvType(),
      isConnectableLegacyAdvertisement(device->getAdvType()) ? "true" : "false");
    if (isConnectableLegacyAdvertisement(device->getAdvType())) break;
  }
  if (!matched) Serial.printf("Station %u: recovery advertisement not seen; retry saved Random address.\n", station.id);
  scan->clearResults();
}

bool accessPower(const Target* target, int& observed, char* error, int& code) {
  waitForCleanup();
  code = 0;
  auto* client = NimBLEDevice::createClient();
  if (!client) { strcpy(error, "BLE_NO_CLIENT"); return false; }
  client->setConnectTimeout(3000); client->setConnectRetries(1);
  bool ok = false;
  if (!client->connect(NimBLEAddress(std::string(station.mac), 1))) strcpy(error, "BLE_CONNECT_FAILED");
  else {
    auto* service = client->getService(kService);
    auto* power = service ? service->getCharacteristic(kPower) : nullptr;
    if (!power || (target && !power->canWrite() && !power->canWriteNoResponse())) strcpy(error, "POWER_CHARACTERISTIC_MISSING");
    else {
      ok = !target;
      if (target) {
        const uint8_t command = static_cast<uint8_t>(*target);
        ok = power->writeValue(&command, 1, !power->canWriteNoResponse());
        if (!ok) strcpy(error, "BLE_WRITE_FAILED");
      }
      if (ok && power->canRead()) {
        if (target) delay(250);
        const auto value = power->readValue();
        if (value.size()) observed = static_cast<uint8_t>(value[0]);
      }
      if (ok && observed < 0) { strcpy(error, "BLE_READ_FAILED"); if (!target) ok = false; }
    }
  }
  if (!ok || observed < 0) code = client->getLastError();
  NimBLEDevice::deleteClient(client); // May be deferred; never touch client again.
  return ok;
}

bool superseded(uint32_t generation, bool readOnly) {
  Lock lock;
  return measured.generation != generation || (readOnly && measured.pending);
}

void worker(void*) {
  for (;;) {
    waitForCleanup();
    uint32_t generation = 0;
    Target target = Target::Sleep;
    bool work = false, readOnly = true;
    {
      Lock lock;
      const uint32_t now = millis();
      if (measured.pending) {
        work = true; readOnly = false; target = measured.requested;
        measured.pending = false; measured.busy = true;
      } else if (!measured.lastReadAttemptMs || now - measured.lastReadAttemptMs >= readInterval(measured, now)) {
        work = true; measured.reading = true;
      }
      generation = measured.generation;
    }
    if (work) {
      int observed = -1, code = 0;
      char error[96]{};
      bool sent = false;
      for (unsigned attempt = 0; attempt < (readOnly ? 2U : 3U); ++attempt) {
        if (superseded(generation, readOnly)) break;
        if (attempt) {
          delay(readOnly ? 350 + esp_random() % 301 : 600 * attempt + esp_random() % 301);
          if (superseded(generation, readOnly)) break;
          if (code == BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT)) recoverAddress();
          if (superseded(generation, readOnly)) break;
        }
        error[0] = '\0';
        sent = accessPower(readOnly ? nullptr : &target, observed, error, code);
        if (sent) {
          Serial.printf("Station %u: %s OK, worker_attempt=%u, observed=%d\n",
            station.id, readOnly ? "read" : targetName(target), attempt + 1, observed);
          break;
        }
        Serial.printf("Station %u: %s, BLE code=%d, worker_attempt=%u, address_type=1, host_retry_limit=1\n",
          station.id, error, code, attempt + 1);
      }
      {
        Lock lock;
        // Record the measurement before deferred client cleanup can delay us.
        if (readOnly) finishRead(measured, generation, observed, millis(), error, code);
        else { finish(measured, generation, sent, observed, millis(), error); measured.bleError = code; }
      }
    }
    delay(work ? 150 : 20);
  }
}

// Replace SDK command handlers rather than treating requested values as readings.
// A successful response accepts queued work; only BLE observations update OnOff.
esp_err_t powerCommand(const chip::app::ConcreteCommandPath& path, chip::TLV::TLVReader& reader, void* opaque) {
  using chip::Protocols::InteractionModel::Status;
  bool valid = false, on = false;
  if (path.mCommandId == OnOff::Commands::On::Id) {
    OnOff::Commands::On::DecodableType data;
    valid = chip::app::DataModel::Decode(reader, data) == CHIP_NO_ERROR; on = true;
  } else if (path.mCommandId == OnOff::Commands::Off::Id) {
    OnOff::Commands::Off::DecodableType data;
    valid = chip::app::DataModel::Decode(reader, data) == CHIP_NO_ERROR;
  } else if (path.mCommandId == OnOff::Commands::Toggle::Id) {
    OnOff::Commands::Toggle::DecodableType data;
    valid = chip::app::DataModel::Decode(reader, data) == CHIP_NO_ERROR;
    Lock lock; on = !measuredOn(measured);
  }
  const bool accepted = valid && workerStarted;
  if (accepted) { Lock lock; request(measured, on ? Target::On : Target::Sleep, millis()); }
  static_cast<chip::app::CommandHandler*>(opaque)->AddStatus(path, accepted ? Status::Success : Status::Failure);
  Serial.printf("Matter command: station=%u, accepted=%s, target=%s\n", station.id,
    accepted ? "true" : "false", on ? "ON" : "SLEEP");
  return ESP_OK;
}

class MeasuredPowerHandler : public chip::app::CommandHandlerInterface {
public:
  explicit MeasuredPowerHandler(uint16_t endpoint)
    : CommandHandlerInterface(chip::MakeOptional(endpoint), OnOff::Id) {}
  void InvokeCommand(HandlerContext& context) override {
    context.SetCommandHandled(); // Never fall through to the SDK's optimistic/timed handlers.
    powerCommand(context.mRequestPath, context.GetReader(), &context.mCommandHandler);
  }
};

void reportMeasurement(intptr_t) {
  bool reachable, on;
  { Lock lock; reachable = freshMeasurement(measured, millis()); on = measuredOn(measured); }
  static bool initialized = false, lastOn = false, lastReachable = false;
  if (initialized && reachable == lastReachable && on == lastOn) return;
  auto onValue = esp_matter_bool(on);
  auto reachableValue = esp_matter_bool(reachable);
  const uint16_t endpoint = accessory.getEndPointId();
  reportingMeasurement = true;
  const auto onResult = esp_matter::attribute::update(endpoint, OnOff::Id, OnOff::Attributes::OnOff::Id, &onValue);
  const auto reachableResult = esp_matter::attribute::update(endpoint, BridgedDeviceBasicInformation::Id,
    BridgedDeviceBasicInformation::Attributes::Reachable::Id, &reachableValue);
  reportingMeasurement = false;
  if (onResult == ESP_OK && reachableResult == ESP_OK) {
    initialized = true; lastOn = on; lastReachable = reachable;
    Serial.printf("Matter measurement: station=%u, on=%s, reachable=%s\n", station.id,
      on ? "true" : "false", reachable ? "true" : "false");
  } else Serial.printf("Matter report failed: on_code=%d, reachable_code=%d\n", onResult, reachableResult);
}

bool loadStation() {
  Preferences saved;
  if (!saved.begin("lhbridge", true)) return false;
  const String json = saved.getString("stations", ""); saved.end();
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, json)) return false;
  StationConfig stations[kStations]; std::string error;
  const bool legacy = doc.is<JsonArray>();
  uint32_t counter = legacy ? 6 : doc["next_id"].as<uint32_t>();
  if (!parseStationList(legacy ? doc.as<JsonVariantConst>() : doc["stations"].as<JsonVariantConst>(),
      stations, nullptr, counter, true, legacy, error)) return false;
  for (const auto& item : stations) if (item.id && item.mac[0]) { station = item; return true; }
  return false;
}

bool beginMatter() {
  using namespace esp_matter;
  // Test-only product identity; commissioning credentials are generated per board.
  Matter.selectNetwork(MATTER_NETWORK_WIFI, true);
  Matter.setVendorName("Lighthouse Bridge");
  Matter.setProductName("Lighthouse Matter Prototype");
  Matter.setDeviceName("Lighthouse Matter Test");
  Preferences own;
  if (!own.begin("lhmatter", false)) { setupError = "MATTER_STORAGE_FAILED"; return false; }
  setupPasscode = own.getUInt("passcode", 0);
  setupDiscriminator = own.getUShort("discriminator", esp_random() & 0xfff);
  if (!setupPasscode || !Matter.setSetupPasscode(setupPasscode)) {
    do { setupPasscode = 1 + esp_random() % 99999998; } while (!Matter.setSetupPasscode(setupPasscode));
    if (own.putUInt("passcode", setupPasscode) != sizeof(setupPasscode) ||
        own.putUShort("discriminator", setupDiscriminator) != sizeof(setupDiscriminator)) {
      own.end(); setupError = "MATTER_STORAGE_FAILED"; return false;
    }
  }
  own.end(); Matter.setSetupDiscriminator(setupDiscriminator);
  if (!accessory.begin(false)) { setupError = "MATTER_ENDPOINT_FAILED"; return false; }
  // Guard against any SDK path bypassing our command handler: only this task's
  // measurement reports may change OnOff. Such a bypass must fail, not invent state.
  accessory.onChange([](bool) { return reportingMeasurement.load(); });
  auto* endpoint = endpoint::get(node::get(), accessory.getEndPointId());
  endpoint::aggregator::config_t aggregateConfig;
  auto* aggregate = endpoint::aggregator::create(node::get(), &aggregateConfig, ENDPOINT_FLAG_NONE, nullptr);
  endpoint::bridged_node::config_t bridgedConfig;
  bridgedConfig.bridged_device_basic_information.reachable = false;
  if (!aggregate || endpoint::bridged_node::add(endpoint, &bridgedConfig) != ESP_OK ||
      endpoint::set_parent_endpoint(endpoint, aggregate) != ESP_OK) {
    setupError = "MATTER_BRIDGE_FAILED"; return false;
  }
  auto* basic = cluster::get(endpoint, BridgedDeviceBasicInformation::Id);
  size_t labelLength = std::min(strlen(station.name), static_cast<size_t>(32));
  while (labelLength && (static_cast<uint8_t>(station.name[labelLength]) & 0xc0) == 0x80) --labelLength;
  String unique = WiFi.macAddress(); unique.replace(":", "");
  unique = "lhm-" + unique + "-" + String(station.hueId);
  if (!cluster::bridged_device_basic_information::attribute::create_node_label(basic, station.name, labelLength) ||
      !cluster::bridged_device_basic_information::attribute::create_unique_id(basic,
        const_cast<char*>(unique.c_str()), unique.length())) {
    setupError = "MATTER_IDENTITY_FAILED"; return false;
  }
  static MeasuredPowerHandler handler(accessory.getEndPointId());
  if (chip::app::CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(&handler) != CHIP_NO_ERROR) {
    setupError = "MATTER_COMMAND_FAILED"; return false;
  }
  Matter.begin(); matterStarted = true;
  if (chip::app::CommandHandlerInterfaceRegistry::Instance().GetCommandHandler(accessory.getEndPointId(), OnOff::Id) != &handler) {
    setupError = "MATTER_HANDLER_CHANGED"; return false;
  }
  manualCode = Matter.getManualPairingCode();
  if (!manualCode.length()) { setupError = "MATTER_START_FAILED"; return false; }
  if (!NimBLEDevice::init("Lighthouse Matter")) { setupError = "BLE_INIT_FAILED"; return false; }
  if (xTaskCreatePinnedToCore(worker, "lighthouse-ble", 8192, nullptr, 1, nullptr, 0) != pdPASS) {
    setupError = "BLE_WORKER_FAILED"; return false;
  }
  workerStarted = true; setupError = "";
  Serial.printf("Matter prototype ready: station=%u; no power command sent on startup.\n", station.id);
  return true;
}

void emitInfo() {
  StaticJsonDocument<1024> doc;
  doc["protocol"] = 1; doc["version"] = kVersion;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["station_id"] = stationLoaded ? station.id : 0;
  doc["station_name"] = stationLoaded ? station.name : "";
  doc["ready"] = workerStarted.load();
  doc["paired"] = matterStarted && Matter.isDeviceCommissioned();
  if (workerStarted && !doc["paired"].as<bool>()) doc["pairing_code"] = manualCode;
  else doc["pairing_code"] = nullptr;
  doc["error"] = setupError;
  {
    Lock lock;
    const uint32_t now = millis();
    doc["uptime_ms"] = now; doc["reachable"] = freshMeasurement(measured, now);
    doc["observed_raw"] = measured.observed; doc["observed_ms"] = measured.observedMs;
    doc["command_status"] = commandStatus(measured, now);
  }
  Serial.print("LH_MATTER_INFO "); serializeJson(doc, Serial); Serial.println();
}

void setup() {
  Serial.begin(115200);
  stateMutex = xSemaphoreCreateMutex();
  if (!stateMutex) abort();
  stationLoaded = loadStation();
  if (!stationLoaded) { setupError = "CONFIGURE_STABLE_FIRST"; return; }
  WiFi.setSleep(true); // Set Arduino's cached coexistence policy before STA starts.
  WiFi.mode(WIFI_STA); WiFi.enableIPv6(); WiFi.begin();
  Serial.printf("Lighthouse Matter prototype %s; waiting for saved Wi-Fi.\n", kVersion);
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
  if (connected && !wifiWasConnected) {
    if (esp_wifi_set_ps(WIFI_PS_MIN_MODEM) != ESP_OK) { setupError = "WIFI_SLEEP_FAILED"; attempted = true; }
  }
  wifiWasConnected = connected;
  if (connected && stationLoaded && !attempted) { attempted = true; beginMatter(); }
  static uint32_t lastReport = 0;
  if (workerStarted && millis() - lastReport >= 500) {
    lastReport = millis(); chip::DeviceLayer::PlatformMgr().ScheduleWork(reportMeasurement, 0);
  }
  delay(10);
}
