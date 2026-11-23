#include <Arduino.h>
#include <ArduinoJson.h>
#include "rs_runtime.h"
#include <ESPmDNS.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>
#include <memory>
#include "station_config.h"
#include "ota_receiver.h"
#include "bridge_core.h"
#include "web_ui_gzip.h"
#include "status_api.h"
#include "startup_sequence.h"
#include "config_stream.h"
#include "matter_http.h"
#include "http_body.h"
#include <esp_mac.h>
#include <esp_heap_caps.h>
#include <atomic>
#include "password_hash.h"
#include "web_account.h"
#include "local_wifi_page.h"
#include "rs_wifi_portal.h"
#include "network_mode.h"
#include "gpio_buttons.h"
#include "remote_buttons.h"
#include <esp_sleep.h>
#include <driver/rtc_io.h>
bool localWifiMode = false, localApReady = false, wifiSetupBoot = false;
std::atomic<uint32_t> wifiApEvent{0}, wifiApJoins{0}, wifiApLeaves{0}, wifiApReason{0};
String portalModeToken;
bool webUsernameEnabled = false;
String webUsername;
String roomName;
bool maintenanceWaitIdle=false;
bool remoteOnly = false;
int identifySlot = -1;
bool stationInfoJob = false;
uint32_t identifyJob = 0;
const char* identifyResult = nullptr;

#if MBEDTLS_VERSION_MAJOR >= 3
#define LH_SHA_START mbedtls_sha256_starts
#define LH_SHA_UPDATE mbedtls_sha256_update
#define LH_SHA_FINISH mbedtls_sha256_finish
#else
#define LH_SHA_START mbedtls_sha256_starts_ret
#define LH_SHA_UPDATE mbedtls_sha256_update_ret
#define LH_SHA_FINISH mbedtls_sha256_finish_ret
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C3)
#define LH_BOOT_PIN 9
#else
#define LH_BOOT_PIN 0
#endif
using namespace bridge;
MatterWebServer server(80);
static_assert(CONFIG_BT_NIMBLE_MAX_CONNECTIONS == 1,"The sequential Matter worker needs one BLE connection");
static_assert(!CONFIG_BT_NIMBLE_ROLE_PERIPHERAL && !CONFIG_BT_NIMBLE_ROLE_BROADCASTER &&
              CONFIG_BT_NIMBLE_ROLE_CENTRAL && CONFIG_BT_NIMBLE_ROLE_OBSERVER,"Use only the Lighthouse client/scanner roles");
Preferences prefs;
SemaphoreHandle_t stateMutex;
StationConfig configs[kStations];
StationState states[kStations];
std::atomic<bool> bleWorkerAvailable{false};
std::atomic<const char*> bleStartupError{"BLE_STARTING"};
ButtonBinding gpioButtons[2];
ButtonDebouncer gpioDebounce[2];
void applyButtonPins() {
  for (unsigned i=0;i<2;++i) {
    gpioDebounce[i] = ButtonDebouncer{};
    if (gpioButtons[i].pin >= 0) pinMode(gpioButtons[i].pin, INPUT_PULLUP);
  }
}
bool parseButtons(JsonVariantConst value, ButtonBinding (&out)[2]) {
  if (!value.is<JsonArrayConst>() || value.size()!=2) return false;
  for (unsigned i=0;i<2;++i) {
    const auto item=value[i];
    if (!item["pin"].is<int>() || !item["action"].is<const char*>()) return false;
    const int pin=item["pin"].as<int>();
    if (pin < -1 || pin > 48) return false;
    out[i].pin=pin;
    const char* action=item["action"];
    if (!strcmp(action,"disabled")) out[i].action=ButtonAction::Disabled;
    else if (!strcmp(action,"all_on")) out[i].action=ButtonAction::AllOn;
    else if (!strcmp(action,"all_off")) out[i].action=ButtonAction::AllOff;
    else return false;
  }
  return validButtonBindings(out);
}
uint32_t nextHueId = 1;
String customDeviceName;
bool otaPrepared = false, otaUploading = false, otaSucceeded = false, otaCandidateReady = false, otaRequestAuthorized = false;
void emitDeviceInfo();
String otaToken, otaOwner, otaError;
uint32_t otaPreparedAt = 0, restartAt = 0;
bool restartPending = false;
String accessKey, apName, hostname;
String webSalt, webHash;
bool usingLocalWifi() {
  return localWifiMode;
}
IPAddress bridgeAddress() { return usingLocalWifi() ? WiFi.softAPIP() : WiFi.localIP(); }
struct WebSession { String token; uint32_t seen = 0; bool bootstrap = false; };
WebSession sessions[4];
unsigned loginFailures = 0;
uint32_t loginBlockedAt = 0;
bool loginBlocked = false;
PortalWifiManager wifiManager;
bool servicesStarted = false, manualPortal = false;
bool blePaused = false;
bool bleActive = false;
bool portalRequested = false;
bool alexaReady = false;
bool scanPending = false, scanning = false;
bool refreshRequested[kStations]{};
uint32_t scanFinishedMs = 0;
using FoundStation = StatusDiscovery;
FoundStation found[16]{};
#include "matter_scan.h"
unsigned foundCount = 0;
bool foundTruncated = false;

struct Lock {
  Lock() { xSemaphoreTake(stateMutex, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(stateMutex); }
};

uint32_t matterDataFree() { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); }
uint32_t matterDataLargest() { return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); }
struct JsonHttpSink {
  size_t write(const uint8_t* data, size_t size) {
    return server.writeBytes(data,size);
  }
};
void restartBridge() {
  // rs-matter commits every persistence mutation before acknowledging it.
  ESP.restart();
}
void sendJson(int code, const JsonDocument& doc) {
  if (doc.overflowed()) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(500, "application/json", "{\"error\":\"OUT_OF_MEMORY\"}");
    return;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(measureJson(doc));
  server.send(code,"application/json; charset=utf-8","");
  JsonHttpSink sink; JsonStreamWriter writer(sink);
  serializeJson(doc,writer);
  if (!writer.flush()) server.client().stop();
}
void fail(int code, const char* message) {
  StaticJsonDocument<256> doc;
  doc["error"] = message;
  sendJson(code, doc);
}
int sessionIndex() {
  const String cookies = server.header("Cookie");
  int position = 0;
  while (position < static_cast<int>(cookies.length())) {
    int end = cookies.indexOf(';', position);
    if (end < 0) end = cookies.length();
    String item = cookies.substring(position, end); item.trim();
    if (item.startsWith("LHSESSION=")) {
      const String token = item.substring(10);
      for (unsigned i = 0; i < 4; ++i) {
        if (sessions[i].token.length() && equalSecret(sessions[i].token, token) &&
            millis() - sessions[i].seen < 12UL * 60 * 60 * 1000) {
          sessions[i].seen = millis(); return i;
        }
      }
    }
    position = end + 1;
  }
  return -1;
}
void issueSession(bool bootstrap) {
  unsigned selected = 0;
  for (unsigned i = 0; i < 4; ++i) {
    if (!sessions[i].token.length()) { selected = i; break; }
    if (millis() - sessions[i].seen > millis() - sessions[selected].seen) selected = i;
  }
  sessions[selected].token = randomHex(24);
  sessions[selected].seen = millis();
  sessions[selected].bootstrap = bootstrap;
  server.sendHeader("Set-Cookie", "LHSESSION=" + sessions[selected].token +
                    "; Path=/; HttpOnly; SameSite=Strict; Max-Age=43200");
}
bool authenticated(bool adminOnly = false, bool allowBootstrap = false) {
  const int index = sessionIndex();
  if (index >= 0) {
    if (sessions[index].bootstrap && !allowBootstrap) { fail(428, "PASSWORD_REQUIRED"); return false; }
    return true;
  }
  if (!adminOnly && accessKey.length() && equalSecret(server.header("X-API-Key"), accessKey)) return true;
  fail(401, "LOGIN_REQUIRED"); return false;
}
bool sameOriginJson() {
  const String origin = server.header("Origin");
  if (origin.length() && origin != "http://" + server.hostHeader()) {
    fail(403, "ORIGIN_DENIED"); return false;
  }
  if (!server.header("Content-Type").startsWith("application/json")) {
    fail(415, "JSON_REQUIRED"); return false;
  }
  return true;
}
bool mutationAllowed() {
  return authenticated() && sameOriginJson();
}
bool readBody(JsonDocument& doc) {
  if (server.arg("plain").length() > 8192) { fail(413, "REQUEST_TOO_LARGE"); return false; }
  if (deserializeJson(doc, server.arg("plain"))) { fail(400, "JSON_INVALID"); return false; }
  return true;
}
bool occupied(bool includeRead = true) { // Caller holds stateMutex.
  if (identifySlot >= 0) return true;
  if (scanning || scanPending) return true;
  if (includeRead && bleActive) return true;
  for (auto& state : states) if (state.busy || state.pending || (includeRead && state.reading)) return true;
  return false;
}
bool queueTarget(int slot, Target target, bool configuredOff = false) {
  if (!bleWorkerAvailable.load()) return false;
  Lock lock;
  bool accepted = false;
  if (blePaused || portalRequested || otaPrepared || restartPending) return false;
  for (unsigned i = 0; i < kStations; ++i) {
    if ((slot < 0 || slot == static_cast<int>(configs[i].id)) && configs[i].mac[0]) {
      request(states[i], configuredOff ? offTarget(configs[i]) : target, millis());
      accepted = true;
    }
  }
  return accepted;
}


// BLE is confined to this worker: scan, connect, write and disconnect never
// block the HTTP and Matter loop. One connection at a time for up to 16 stations.
void waitForBleCleanup() {
  // In NimBLE 2.x deleteClient() can defer destruction until disconnection.
  // Never dereference that client again, or start another scan/connection yet.
  const uint32_t started = millis();
  bool warned = false;
  while (NimBLEDevice::getCreatedClientCount() != 0) {
    if (!warned && millis() - started >= 5000) {
      Serial.println("BLE disconnect cleanup is still pending; worker waits before further radio work.");
      warned = true;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void refreshRetryAddress(StationConfig& config) {
  waitForBleCleanup();
  MatterScanCapture capture; capture.assigned = config.mac;
  const int code = runMatterScan(capture,false,2000);
  const auto& result = capture.recovery;
  if (code) Serial.printf("Station %u: retry scan failed, BLE code=%d\n",config.id,code);
  if (result.seen) {
    config.addressType = 1;
    Serial.printf("Station %u: retry scan: advertisement seen, RSSI=%d dBm, address_type=1, advertised_address_type=%u, adv_type=%u\n",
      config.id,result.rssi,result.addressType,result.type);
  } else if (result.matched) {
    Serial.printf("Station %u: retry scan: address seen, RSSI=%d dBm, adv_type=%u, advertised_address_type=%u (not connectable)\n",
      config.id,result.rssi,result.type,result.addressType);
  } else Serial.printf("Station %u: retry scan: assigned address not seen within 2 seconds; reports_seen=%u\n",
      config.id,result.reports);
}

void logBleFailure(unsigned slot, const char* error, int code, unsigned attempt, uint8_t addressType) {
  Serial.printf("Station %u: %s, BLE code=%d, worker_attempt=%u, address_type=%u, host_retry_limit=1%s\n", slot,
                error, code, attempt + 1, addressType,
                code == BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT)
                  ? " (HCI 0x3e: connection establishment failed)"
                  : code == BLE_HS_ETIMEOUT ? " (connection timeout)" : "");
}

#include "rs_station_info.h"
bool accessPower(const StationConfig& config, const Target* target, int& observed, char* error, int& bleError
                 , bool identify = false, bool info = false
                 ) {
  waitForBleCleanup();
  bleError = 0;
  if (matterDataFree() < 4096 || matterDataLargest() < sizeof(NimBLEClient)+128) {
    strcpy(error,"BLE_NO_MEMORY"); bleError = BLE_HS_ENOMEM; return false;
  }
  NimBLEClient* client = NimBLEDevice::createClient();
  if (!client) { strcpy(error, "BLE_NO_CLIENT"); return false; }
  // The host retries HCI 0x3e before exposing a failed connection to the worker.
  // Explicitly bound nested retries; NimBLE 2.x expects milliseconds here.
  client->setConnectTimeout(3000);
  client->setConnectRetries(1);
  bool ok = false;
  if (!client->connect(NimBLEAddress(std::string(config.mac), config.addressType))) {
    strcpy(error, "BLE_CONNECT_FAILED");
  } else {
    auto* service = client->getService(kService);
    if (info) {
      ok = readStationInfo(client, service);
      NimBLEDevice::deleteClient(client);
      return ok;
    }
    if (identify) {
      auto* light = service ? service->getCharacteristic("00008421-1212-efde-1523-785feabcd124") : nullptr;
      const uint8_t flash = 1;
      if (light && (light->canWrite() || light->canWriteNoResponse())) {
        ok = light->writeValue(&flash, 1, light->canWrite());
        if (!ok) strcpy(error, "BLE_WRITE_FAILED");
      } else strcpy(error, "IDENTIFY_UNSUPPORTED");
      if (!ok) bleError = client->getLastError();
      NimBLEDevice::deleteClient(client);
      return ok;
    }
    auto* power = service ? service->getCharacteristic(kPower) : nullptr;
    if (!power || (target && !power->canWriteNoResponse() && !power->canWrite())) {
      strcpy(error, "POWER_CHARACTERISTIC_MISSING");
    } else {
      ok = !target;
      if (target) {
        const uint8_t command = static_cast<uint8_t>(*target); // ON/SLEEP; rs-matter also supports explicit STANDBY.
        ok = power->writeValue(&command, 1, !power->canWriteNoResponse());
        if (!ok) strcpy(error, "BLE_WRITE_FAILED");
      }
      if (ok && power->canRead()) {
        if (target) vTaskDelay(pdMS_TO_TICKS(250));
        const auto value = power->readValue();
        if (value.size() > 0) observed = static_cast<uint8_t>(value[0]);
      }
      if (ok && observed < 0) {
        strcpy(error, "BLE_READ_FAILED");
        if (!target) ok = false;
      }
    }
  }
  if (!ok || observed < 0) bleError = client->getLastError();
  NimBLEDevice::deleteClient(client);
  return ok;
}

void discoverStations() {
  waitForBleCleanup();
  Serial.println("BLE discovery: started (read-only, 5000 ms scan).");
  FoundStation next[16]{};
  MatterScanCapture capture; capture.entries = next;
  const int code = runMatterScan(capture,true,5000);
  const unsigned count = capture.count;
  const bool truncated = capture.truncated;
  if (code) {
    { Lock lock; scanning = false; }
    Serial.printf("BLE discovery: failed, BLE code=%d\n",code);
    return; // Retain earlier discovery results/timestamp on an incomplete scan.
  }
  {
    Lock lock;
    memcpy(found, next, sizeof(found));
    foundCount = count;
    foundTruncated = truncated;
    scanning = false;
    scanFinishedMs = millis();
  }
  Serial.printf("BLE discovery: completed, found=%u, truncated=%s\n",count,truncated ? "true" : "false");
}

void bleWorker(void*) {
  while (!bleWorkerAvailable.load()) vTaskDelay(pdMS_TO_TICKS(20));
  unsigned cursor = 0;
  for (;;) {
    waitForBleCleanup();
    int selected = -1;
    StationConfig config;
    Target target = Target::Sleep;
    uint32_t generation = 0;
    bool doScan = false;
    bool readOnly = false;
    bool identify = false, info = false;
    {
      Lock lock;
      if (blePaused) { /* Portal owns the radio; do not schedule new BLE work. */ }
      else {
      for (unsigned offset = 0; offset < kStations; ++offset) {
        const unsigned i = (cursor + offset) % kStations;
        if (!states[i].pending) continue;
        selected = i;
        config = configs[i];
        target = states[i].requested;
        generation = states[i].generation;
        states[i].pending = false;
        states[i].busy = true;
        break;
      }
      if (selected < 0 && identifySlot >= 0) {
        selected = identifySlot;
        config = configs[selected];
        identify = true;
        info = stationInfoJob;
      }
      if (selected < 0 && scanPending) {
        scanPending = false;
        scanning = doScan = true;
      }
      if (selected < 0 && !doScan) {
        for (unsigned offset = 0; offset < kStations; ++offset) {
          const unsigned i = (cursor + offset) % kStations;
          const auto& state = states[i];
          if (remoteOnly && !awaitingConfirmation(state, millis())) continue;
          const uint32_t interval = readInterval(state, millis());
          if (!configs[i].mac[0] || (!refreshRequested[i] && state.lastReadAttemptMs &&
              millis() - state.lastReadAttemptMs < interval)) continue;
          selected = i; config = configs[i]; readOnly = true;
          generation = states[i].generation;
          refreshRequested[i] = false; states[i].reading = true;
          break;
        }
      }
      }
      bleActive = selected >= 0 || doScan;
    }
    if (doScan) discoverStations();
    if (selected >= 0) {
      char error[96]{};
      int observed = -1;
      bool sent = false;
      int bleError = 0;
      if (identify) {
        const bool completed = accessPower(config, nullptr, observed, error, bleError, !info, info);
        waitForBleCleanup();
        Lock lock;
        if (completed && info && !stationInfo.name[0]) {
          for (unsigned i = 0; i < foundCount; ++i) {
            if (!strcmp(found[i].mac, config.mac)) {
              snprintf(stationInfo.name, sizeof stationInfo.name, "%s", found[i].name);
              break;
            }
          }
        }
        identifyResult = completed ? "" : info ? "STATION_INFO_FAILED" : !strcmp(error, "IDENTIFY_UNSUPPORTED")
                         ? "IDENTIFY_UNSUPPORTED" : "IDENTIFY_FAILED";
        identifySlot = -1;
      } else
      if (readOnly) {
        // Retry one failed read without changing power. Yield to pending user
        // commands between attempts, and let the device finish disconnecting.
        for (unsigned attempt = 0; attempt < 2; ++attempt) {
          if (attempt) {
            bool urgent = false;
            { Lock lock; for (auto& state : states) urgent |= state.pending;
              urgent |= blePaused || states[selected].generation != generation; }
            if (urgent) break;
            // Vary timing so repeated attempts do not stay synchronized with
            // a recurring Wi-Fi/radio event. Never send power while recovering.
            vTaskDelay(pdMS_TO_TICKS(350 + esp_random() % 301));
            if (bleError == BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT)) {
              refreshRetryAddress(config);
            }
            { Lock lock; for (auto& state : states) urgent |= state.pending;
              urgent |= blePaused || states[selected].generation != generation; }
            if (urgent) break;
          }
          error[0] = '\0';
          if (accessPower(config, nullptr, observed, error, bleError)) {
            Serial.printf("Station %d: read OK, worker_attempt=%u, observed=%d\n",
                          config.id, attempt + 1, observed);
            break;
          }
          logBleFailure(config.id, error, bleError, attempt, config.addressType);
        }
        bool becameConfirmed = false;
        {
          Lock lock;
          const bool wasConfirmed = states[selected].commandConfirmed;
          finishRead(states[selected], generation, observed, millis(), error, bleError);
          becameConfirmed = !wasConfirmed && states[selected].commandConfirmed;
        }
        if (becameConfirmed) Serial.printf("Station %d: command target confirmed, observed=%d\n", config.id, observed);
      } else {
      for (unsigned attempt = 0; attempt < 3; ++attempt) {
        { Lock lock; if (states[selected].generation != generation) break; }
        if (attempt && bleError == BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT)) {
          refreshRetryAddress(config);
          { Lock lock; if (states[selected].generation != generation) break; }
        }
        error[0] = '\0';
        sent = accessPower(config, &target, observed, error, bleError);
        if (sent) break;
        logBleFailure(config.id, error, bleError, attempt, config.addressType);
        if (attempt < 2) vTaskDelay(pdMS_TO_TICKS(600 * (attempt + 1) + esp_random() % 301));
      }
      bool confirmed = false;
      {
        Lock lock;
        finish(states[selected], generation, sent, observed, millis(), error);
        states[selected].bleError = bleError;
        confirmed = states[selected].commandConfirmed && states[selected].generation == generation;
      }
      Serial.printf("Station %d: %s, %s, observed=%d, confirmed=%s\n", config.id,
                    targetName(target), sent ? "sent" : error, observed, confirmed ? "true" : "false");
      }
      cursor = (selected + 1) % kStations;
    }
    bool stopRemoteWorker;
    {
      Lock lock;
      bleActive = NimBLEDevice::getCreatedClientCount() != 0;
      stopRemoteWorker = remoteOnly && blePaused && !bleActive;
    }
    if (stopRemoteWorker) {
      bleWorkerAvailable = false;
      vTaskDelete(nullptr);
    }
    vTaskDelay(pdMS_TO_TICKS(selected >= 0 ? 150 : 20));
  }
}

#include "matter_config_io.h"
#include "rs_matter_adapter.h"
#include "matter_status.h"
#include "matter_config_route.h"

void statusRoute() {
  matterStatusRoute();
}


struct FirmwareOtaIO {
  mbedtls_sha256_context sha;
  bool hashing = false;
  bool begin(uint32_t size) {
    if (!Update.begin(size, U_FLASH)) return false;
    mbedtls_sha256_init(&sha); hashing = true;
    if (LH_SHA_START(&sha, 0)) { abort(); return false; }
    return true;
  }
  bool write(uint8_t* bytes, size_t count) {
    return !LH_SHA_UPDATE(&sha, bytes, count) && Update.write(bytes, count) == count;
  }
  bool finishHash(uint8_t* digest) {
    const bool ok = !LH_SHA_FINISH(&sha, digest);
    mbedtls_sha256_free(&sha); hashing = false;
    return ok;
  }
  void abort() {
    Update.abort();
    if (hashing) { mbedtls_sha256_free(&sha); hashing = false; }
  }
} otaIo;
OtaReceiver<FirmwareOtaIO> otaStream(otaIo);

void cancelOta(const char* error) {
  otaStream.abort();
  otaError = error;
  otaPrepared = otaUploading = otaCandidateReady = false;
  otaToken = otaOwner = "";
  blePaused = false;
}
bool sameOtaOwner() {
  const int index = sessionIndex();
  const String origin = server.header("Origin");
  return index >= 0 && !sessions[index].bootstrap && otaPrepared &&
         equalSecret(sessions[index].token, otaOwner) &&
         equalSecret(server.header("X-OTA-Token"), otaToken) &&
         (!origin.length() || origin == "http://" + server.hostHeader());
}
void acceptOtaBytes(uint8_t* bytes, size_t count) {
  if (!otaError.isEmpty()) return;
  const auto* partition = esp_ota_get_next_update_partition(nullptr);
  if (!partition) { cancelOta("OTA_UNAVAILABLE"); return; }
  const char* error = otaStream.feed(bytes, count, partition->size);
  if (error) cancelOta(error);
}
void otaUpload() {
  lh_rs_pump();
  auto& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaRequestAuthorized = false;
    if (!sameOtaOwner()) { otaError = "LOGIN_REQUIRED"; return; }
    otaRequestAuthorized = true;
    if (otaUploading || otaStream.started || !blePaused || bleActive) { cancelOta("OTA_INVALID"); return; }
    otaSucceeded = otaCandidateReady = false;
    otaUploading = true; otaError = ""; otaStream.reset();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (otaRequestAuthorized && otaUploading && otaError.isEmpty()) acceptOtaBytes(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (otaRequestAuthorized && (otaUploading || otaStream.started)) cancelOta("OTA_ABORTED");
  } else if (upload.status == UPLOAD_FILE_END && otaRequestAuthorized && otaUploading && otaError.isEmpty()) {
    const char* error = otaStream.finish();
    if (error) { cancelOta(error); return; }
    // Do not commit the boot slot until the complete HTTP request is accepted.
    otaUploading = false; otaCandidateReady = true;
  }
}

#include "rs_maintenance.h"
void configureRemoteRoutes();
void configureMaintenanceRoutes() {
  server.on("/api/device", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    if (otaPrepared || blePaused) { fail(409, "BLE_BUSY"); return; }
    if (localWifiMode) { Lock lock; if (occupied(false) || restartPending) { fail(409,"BLE_BUSY"); return; } }
    StaticJsonDocument<256> doc;
    if (!readBody(doc)) return;
    if (!doc["name"].is<const char*>() || !validDeviceName(doc["name"])) { fail(400, "DEVICE_NAME_INVALID"); return; }
    const String name = doc["name"].as<String>();
    if (prefs.putString("dev_name", name) != name.length()) { fail(500, "SAVE_FAILED"); return; }
    customDeviceName = hostname = apName = name;
    WiFi.setHostname(hostname.c_str()); wifiManager.setHostname(hostname);
    matterSetHostname(hostname);
    doc.clear(); doc["saved"] = true; doc["hostname"] = hostname; doc["setup_ssid"] = apName;
    doc["url"] = "http://" + hostname + ".local/";
    if (localWifiMode) { { Lock lock; blePaused = true; } doc["restarting"] = true; restartPending = true; restartAt = millis(); }
    sendJson(200, doc);
    if (localWifiMode) restartAt = millis();
    emitDeviceInfo();
  });
  server.on("/api/ota/prepare", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    if (!esp_ota_get_next_update_partition(nullptr)) { fail(409, "OTA_UNAVAILABLE"); return; }
    { Lock lock; if (otaPrepared || blePaused || occupied(false)) { fail(409, "BLE_BUSY"); return; }
      blePaused = true; otaPrepared = true; }
    otaToken = randomHex(24); otaOwner = sessions[sessionIndex()].token;
    otaPreparedAt = millis(); otaError = ""; otaSucceeded = false; otaRequestAuthorized = false;
    StaticJsonDocument<256> doc; doc["token"] = otaToken; doc["ready"] = !bleActive;
    const auto* partition = esp_ota_get_next_update_partition(nullptr);
    doc["max_bytes"] = partition ? partition->size : 0;
    sendJson(202, doc);
  });
  server.on("/api/ota", HTTP_GET, [] {
    if (!authenticated(true)) return;
    StaticJsonDocument<192> doc; doc["prepared"] = otaPrepared;
    doc["ready"] = otaPrepared && !bleActive && !otaUploading && !otaSucceeded && !otaCandidateReady; doc["uploading"] = otaUploading;
    const auto* partition = esp_ota_get_next_update_partition(nullptr);
    doc["max_bytes"] = partition ? partition->size : 0;
    sendJson(200, doc);
  });
  server.on("/api/ota/cancel", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    if (otaUploading || otaSucceeded || otaCandidateReady) { fail(409, "BLE_BUSY"); return; }
    cancelOta(""); StaticJsonDocument<32> doc; doc["cancelled"] = true; sendJson(200, doc);
  });
  server.on("/api/ota", HTTP_POST, [] {
    if (!authenticated(true)) { if (otaRequestAuthorized) cancelOta("LOGIN_REQUIRED"); otaRequestAuthorized = false; return; }
    if (!otaRequestAuthorized) { fail(403, "LOGIN_REQUIRED"); return; }
    otaRequestAuthorized = false;
    if (otaCandidateReady && otaStream.started) {
      if (!sameOtaOwner()) { cancelOta("LOGIN_REQUIRED"); fail(403, "LOGIN_REQUIRED"); return; }
      if (!Update.end()) { cancelOta("OTA_WRITE_FAILED"); fail(400, "OTA_WRITE_FAILED"); return; }
      otaStream.committed(); otaCandidateReady = false; otaSucceeded = true; otaToken = otaOwner = "";
      StaticJsonDocument<96> doc; doc["installed"] = true; doc["restarting"] = true;
      sendJson(200, doc); restartAt = millis(); restartPending = true;
    } else fail(400, otaError.length() ? otaError.c_str() : "OTA_INVALID");
  }, otaUpload);
}

void configureRoutes() {
  const char* headers[] = {"X-API-Key", "Origin", "Content-Type", "Cookie", "X-OTA-Token"};
  server.collectHeaders(headers, 5);
  configureMaintenanceRoutes();
  configureRsMaintenanceRoutes();
  configureRemoteRoutes();
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.sendHeader("X-Frame-Options", "DENY");
    server.sendHeader("Content-Encoding", "gzip");
    const uint32_t started = millis();
    Serial.printf("Web UI begin: bytes=%u, free_8bit=%u, largest_8bit=%u\n",
                  unsigned(sizeof(WEB_UI_GZIP)), matterDataFree(), matterDataLargest());
    server.setContentLength(sizeof(WEB_UI_GZIP));
    server.send(200, "text/html; charset=utf-8", "");
    const size_t sent = server.writeBytes(WEB_UI_GZIP, sizeof(WEB_UI_GZIP));
    Serial.printf("Web UI end: sent=%u/%u, complete=%s, handler_ms=%u, free_8bit=%u, largest_8bit=%u\n",
                  unsigned(sent), unsigned(sizeof(WEB_UI_GZIP)), sent == sizeof(WEB_UI_GZIP) ? "true" : "false",
                  unsigned(millis() - started), matterDataFree(), matterDataLargest());
  });
  // Public capability only: no account name, key, hash or salt is returned.
  server.on("/api/login", HTTP_GET, [] {
    StaticJsonDocument<64> doc;
    doc["username_enabled"] = webHash.length() && webUsernameEnabled;
    doc["password_set"] = webHash.length() != 0;
    sendJson(200,doc);
  });
  server.on("/api/network", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    StaticJsonDocument<128> doc;
    if (!readBody(doc)) return;
    if (!doc["mode"].is<const char*>() || strcmp(doc["mode"],"home")) { fail(400,"NETWORK_MODE_INVALID"); return; }
    if (!localWifiMode) { doc.clear(); doc["saved"] = true; doc["restarting"] = false; sendJson(200,doc); return; }
    {
      Lock lock;
      if (occupied(false) || otaPrepared || blePaused || restartPending) { fail(409,"BLE_BUSY"); return; }
      if (!saveNetworkMode(prefs,NetworkMode::Home)) { fail(500,"SAVE_FAILED"); return; }
      blePaused = true; restartPending = true; restartAt = millis();
    }
    doc.clear(); doc["saved"] = true; doc["restarting"] = true; sendJson(200,doc);
    restartAt = millis();
  });
  server.on("/api/username", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    StaticJsonDocument<512> doc;
    if (!readBody(doc)) return;
    bool enabled; const char* name;
    if (!parseWebUsername(doc.as<JsonVariantConst>(),enabled,name)) { fail(400,"USERNAME_INVALID"); return; }
    const String nextName = name;
    doc.clear(); doc["salt"] = webSalt; doc["hash"] = webHash;
    doc["username_enabled"] = enabled; doc["username"] = nextName;
    String record; serializeJson(doc,record);
    if (doc.overflowed() || prefs.putString("web_auth",record) != record.length()) { fail(500,"SAVE_FAILED"); return; }
    webUsernameEnabled = enabled; webUsername = nextName;
    for (auto& session : sessions) session.token = "";
    issueSession(false);
    doc.clear(); doc["saved"] = true; sendJson(200,doc);
  });
  server.on("/api/key", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    StaticJsonDocument<128> doc;
    if (!readBody(doc)) return;
    if (!doc["confirm"].is<bool>() || !doc["confirm"].as<bool>()) { fail(400,"KEY_CONFIRM_REQUIRED"); return; }
    if (localWifiMode) {
      Lock lock;
      if (occupied(false) || otaPrepared || blePaused || restartPending) { fail(409,"BLE_BUSY"); return; }
    }
    const String next = randomHex(12);
    if (next.length() != 24 || prefs.putString("access_key",next) != next.length()) { fail(500,"SAVE_FAILED"); return; }
    accessKey = next;
    doc.clear(); doc["api_key"] = accessKey;
    if (localWifiMode) {
      { Lock lock; blePaused = true; }
      doc["restarting"] = true; restartPending = true; restartAt = millis();
    }
    sendJson(200,doc);
    if (localWifiMode) restartAt = millis();
  });
  server.on("/api/login", HTTP_POST, [] {
    if (!sameOriginJson()) return;
    StaticJsonDocument<512> doc;
    if (!readBody(doc)) return;
    if (loginBlocked && millis() - loginBlockedAt < 30000) { fail(429, "LOGIN_THROTTLED"); return; }
    if (loginBlocked) { loginBlocked = false; loginFailures = 0; }
    const String password = doc["password"].as<String>();
    const bool bootstrap = !webHash.length();
    const bool usernameValid = bootstrap || !webUsernameEnabled ||
      (doc["username"].is<const char*>() && equalSecret(doc["username"].as<String>(),webUsername));
    const bool valid = usernameValid && doc["password"].is<const char*>() && password.length() <= 128 &&
      (bootstrap ? equalSecret(password, accessKey) : equalSecret(passwordHash(password, webSalt), webHash));
    if (!valid) {
      if (++loginFailures >= 5) { loginBlocked = true; loginBlockedAt = millis(); }
      fail(401, "LOGIN_FAILED"); return;
    }
    loginFailures = 0;
    issueSession(bootstrap);
    doc.clear(); doc["password_required"] = bootstrap; sendJson(200, doc);
  });
  server.on("/api/buttons", HTTP_GET, [] {
    if (!authenticated(true)) return;
    StaticJsonDocument<1024> doc;
    auto wakePins = doc.createNestedArray("wake_pins");
    for (int pin = 0; pin <= 48; ++pin) if (remoteWakePin(pin)) wakePins.add(pin);
    auto pins=doc.createNestedArray("pins");
    for (int pin=0;pin<=48;++pin) if (buttonPinAllowed(pin)) pins.add(pin);
    auto buttons=doc.createNestedArray("buttons");
    for (const auto& binding:gpioButtons) {
      auto item=buttons.createNestedObject();item["pin"]=binding.pin;
      item["action"]=binding.action==ButtonAction::AllOn?"all_on":binding.action==ButtonAction::AllOff?"all_off":"disabled";
    }
    sendJson(200,doc);
  });
  server.on("/api/buttons", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    StaticJsonDocument<512> doc;
    if (!readBody(doc)) return;
    ButtonBinding proposed[2];
    if (!parseButtons(doc["buttons"],proposed)) { fail(400,"BUTTON_CONFIG_INVALID");return; }
    if (otaPrepared || restartPending || portalRequested) { fail(409,"BLE_BUSY");return; }
    // Persist only canonical fields so accepted extra JSON cannot outgrow the
    // bounded boot parser or alter the saved format.
    doc.clear();auto records=doc.to<JsonArray>();
    for (const auto& binding:proposed) {
      auto item=records.createNestedObject();item["pin"]=binding.pin;
      item["action"]=binding.action==ButtonAction::AllOn?"all_on":binding.action==ButtonAction::AllOff?"all_off":"disabled";
    }
    String record;serializeJson(records,record);
    if (prefs.putString("gpio_buttons",record)!=record.length()) { fail(500,"SAVE_FAILED");return; }
    for (auto& old:gpioButtons) if(old.pin>=0) pinMode(old.pin,INPUT);
    for(unsigned i=0;i<2;++i) gpioButtons[i]=proposed[i];
    applyButtonPins();doc.clear();doc["saved"]=true;sendJson(200,doc);
  });
  server.on("/api/account", HTTP_GET, [] {
    if (!authenticated(true, true)) return;
    StaticJsonDocument<512> doc;
    doc["alexa_protocol"] = "matter"; doc["matter_ready"] = matterReady.load() && bleWorkerAvailable.load();
    doc["password_required"] = !webHash.length();
    doc["setup_ssid"] = apName; doc["hostname"] = hostname;
    doc["device_name"] = customDeviceName;
    doc["username_enabled"] = webUsernameEnabled;
    doc["username"] = webUsername;
    doc["network_mode"] = usingLocalWifi() ? "local" : "home";
    if (webHash.length()) {
      doc["api_key"] = accessKey;
      doc["room_name"] = roomName;
    }
    sendJson(200, doc);
  });
  server.on("/api/password", HTTP_POST, [] {
    if (!authenticated(true, true) || !sameOriginJson()) return;
    StaticJsonDocument<768> doc;
    if (!readBody(doc)) return;
    if (!doc["password"].is<const char*>()) { fail(400, "PASSWORD_LENGTH"); return; }
    const String password = doc["password"];
    if (password.length() < 8 || password.length() > 128) { fail(400, "PASSWORD_LENGTH"); return; }
    if (equalSecret(password, accessKey)) { fail(400, "PASSWORD_MUST_DIFFER"); return; }
    const String salt = randomHex(16), hash = passwordHash(password, salt);
    if (hash.length() != 64) { fail(500, "PASSWORD_HASH_FAILED"); return; }
    doc.clear(); doc["salt"] = salt; doc["hash"] = hash;
    doc["username_enabled"] = webUsernameEnabled; doc["username"] = webUsername;
    String record; serializeJson(doc, record);
    if (prefs.putString("web_auth", record) != record.length()) { fail(500, "SAVE_FAILED"); return; }
    webSalt = salt; webHash = hash;
    for (auto& session : sessions) session.token = "";
    issueSession(false);
    doc.clear(); doc["saved"] = true; sendJson(200, doc);
  });
  server.on("/api/logout", HTTP_POST, [] {
    if (!authenticated(true, true) || !sameOriginJson()) return;
    const int index = sessionIndex(); if (index >= 0) sessions[index].token = "";
    server.sendHeader("Set-Cookie", "LHSESSION=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
    StaticJsonDocument<64> doc; doc["logged_out"] = true; sendJson(200, doc);
  });
  server.on("/api/refresh", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    if (!bleWorkerAvailable.load()) { fail(503,bleStartupError.load()); return; }
    { Lock lock; for (unsigned i = 0; i < kStations; ++i) if (configs[i].mac[0]) refreshRequested[i] = true; }
    StaticJsonDocument<64> doc; doc["accepted"] = true; sendJson(202, doc);
  });
  server.on("/api/status", HTTP_GET, statusRoute);
  server.on("/api/power", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    if (!bleWorkerAvailable.load()) { fail(503,bleStartupError.load()); return; }
    StaticJsonDocument<256> doc;
    if (!readBody(doc)) return;
    Target target;
    const bool configuredOff = doc["state"].is<const char*>() && !strcmp(doc["state"],"off");
    const bool targetValid = configuredOff ? (target = Target::Sleep,true) :
      (doc["state"].is<const char*>() && parseTarget(doc["state"],target,true));
    if (!targetValid) { fail(400, "STATE_INVALID"); return; }
    int slot = -1;
    if (doc["station"].is<int>() && doc["station"].as<int>() >= 1 && doc["station"].as<int>() <= static_cast<int>(kStations))
      slot = doc["station"].as<int>();
    else if (!doc["station"].is<const char*>() || strcmp(doc["station"], "all") != 0) {
      fail(400, "STATION_INVALID"); return;
    }
    if (!queueTarget(slot, target, configuredOff)) { fail(409, "NO_STATION"); return; }
    doc.clear(); doc["accepted"] = true;
    sendJson(202, doc);
  });
  server.on("/api/scan", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    if (!bleWorkerAvailable.load()) { fail(503,bleStartupError.load()); return; }
    bool queued = false;
    { Lock lock; if (blePaused || otaPrepared) { fail(409, "BLE_BUSY"); return; } if (!scanning) { queued = !scanPending; scanPending = true; } }
    if (queued) Serial.println("BLE discovery: queued (read-only).");
    StaticJsonDocument<64> doc; doc["accepted"] = true; sendJson(202, doc);
  });
  server.on("/api/config", HTTP_POST, [] {
    matterSaveConfiguration();
  });
  server.on("/api/wifi", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    { Lock lock; if (occupied(false) || otaPrepared) { fail(409, "BLE_BUSY"); return; }
      if (restartPending || portalRequested) { fail(409,"BLE_BUSY"); return; }
      if (!requestWifiSetup(prefs)) { fail(500,"SAVE_FAILED"); return; }
      blePaused = true; portalRequested = true;
    }
    StaticJsonDocument<96> doc; doc["setup_ssid"] = apName; doc["accepted"] = true;
    sendJson(202, doc);
  });
  server.on("/api/matter", HTTP_GET, [] {
    if (!authenticated(true)) return;
    StaticJsonDocument<1024> doc; matterPrivateInfo(doc, true); sendJson(200, doc);
  });
  server.on("/api/matter/commissioning", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    StaticJsonDocument<64> request;
    if (!readBody(request)) return;
    if (!request.is<JsonObject>() || request.size()) { fail(400, "JSON_INVALID"); return; }
    if (!matterOpenCommissioning()) { fail(409, "MATTER_NOT_READY"); return; }
    StaticJsonDocument<64> doc; doc["accepted"] = true; sendJson(202, doc);
  });
  server.onNotFound([] {
    const String uri = server.uri();
    if(uri=="/api/room"||uri=="/api/maintenance"){
      if(authenticated(true))fail(404,"NOT_FOUND");return;
    }
    if (uri == "/api/status" || uri == "/api/power" || uri == "/api/config" ||
        uri == "/api/scan" || uri == "/api/wifi" || uri == "/api/refresh" ||
        uri == "/api/buttons" || uri == "/api/network" || uri == "/api/key" || uri == "/api/username" || uri == "/api/login" || uri == "/api/logout" || uri == "/api/password" || uri == "/api/account" ||
        uri == "/api/device" || uri == "/api/ota" || uri == "/api/ota/prepare" || uri == "/api/ota/cancel" ||
        uri == "/api/matter" || uri == "/api/matter/commissioning") {
      if (authenticated()) fail(404, "NOT_FOUND");
      return;
    }
    fail(404, "NOT_FOUND");
  });
}

void persistWifiCredentials() {
  wifi_config_t config{};
  if (WiFi.status() != WL_CONNECTED || esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK) return;
  // WiFiManager saves credentials; explicitly retain FLASH storage for this core.
  const esp_err_t storage = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
  const esp_err_t saved = storage == ESP_OK ? esp_wifi_set_config(WIFI_IF_STA, &config) : storage;
  if (saved != ESP_OK) Serial.println("WIFI_SAVE_FAILED");
  memset(&config, 0, sizeof(config));
}

void emitDeviceInfo() {
  StaticJsonDocument<512> doc;
  doc["protocol"] = 1; doc["version"] = LH_FIRMWARE_VERSION;
  doc["api_key"] = accessKey; doc["setup_ssid"] = apName;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["password_set"] = webHash.length() > 0;
  doc["network_mode"] = localWifiMode ? "local" : "home";
  const bool networkAvailable = localWifiMode ? localApReady : WiFi.status() == WL_CONNECTED;
  if (networkAvailable) doc["url"] = "http://" + bridgeAddress().toString() + "/";
  else doc["url"] = nullptr;
  Serial.print("LH_INFO "); serializeJson(doc, Serial); Serial.println();
}

void emitWifiInfo() {
  wifi_config_t config{};
  const esp_err_t code = esp_wifi_get_config(WIFI_IF_AP,&config);
  const bool keyMatches = code == ESP_OK && accessKey.length() == 24 &&
    memcmp(config.ap.password,accessKey.c_str(),24) == 0 && config.ap.password[24] == 0;
  const bool nameMatches = code == ESP_OK && config.ap.ssid_len == apName.length() &&
    memcmp(config.ap.ssid,apName.c_str(),apName.length()) == 0;
  StaticJsonDocument<768> doc;
  doc["protocol"] = 1; doc["version"] = LH_FIRMWARE_VERSION;
  doc["mode"] = static_cast<unsigned>(WiFi.getMode()); doc["setup_boot"] = wifiSetupBoot;
  doc["portal_active"] = wifiManager.getConfigPortalActive();
  doc["ap_ipv4_ready"] = WiFi.softAPIP() != IPAddress(0,0,0,0);
  doc["ap_config_error"] = static_cast<int>(code);
  doc["key_matches"] = keyMatches; doc["name_matches"] = nameMatches;
  doc["auth"] = code == ESP_OK ? static_cast<int>(config.ap.authmode) : -1;
  doc["cipher"] = code == ESP_OK ? static_cast<int>(config.ap.pairwise_cipher) : -1;
  doc["channel"] = code == ESP_OK ? config.ap.channel : 0;
  doc["pmf_required"] = code == ESP_OK && config.ap.pmf_cfg.required;
  doc["clients"] = WiFi.softAPgetStationNum();
  doc["joins"] = wifiApJoins.load(); doc["leaves"] = wifiApLeaves.load();
  doc["last_reason"] = wifiApReason.load();
  doc["free_data_heap"] = matterDataFree(); doc["largest_data_block"] = matterDataLargest();
  memset(&config,0,sizeof(config));
  Serial.print("LH_WIFI_INFO "); serializeJson(doc,Serial); Serial.println();
}
void processSerial() {
  static char line[80]; static unsigned length = 0; static bool overflow = false;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line[length] = 0;
      if (!overflow && strcmp(line, "LH_INFO") == 0) emitDeviceInfo();
      if (!overflow && strcmp(line, "LH_WIFI_INFO") == 0) emitWifiInfo();
      if (!overflow && strcmp(line, "LH_MATTER_INFO") == 0) emitMatterInfo();
      if (!overflow && strcmp(line, "LH_RESET_WEB_PASSWORD") == 0) {
        prefs.remove("web_auth"); webSalt = ""; webHash = "";
        webUsernameEnabled = false; webUsername = "";
        for (auto& session : sessions) session.token = "";
        Serial.println("LH_PASSWORD_RESET");
      }
      length = 0; overflow = false;
    } else if (length < sizeof(line) - 1 && !overflow) line[length++] = c;
    else overflow = true;
  }
}

bool loadStations() {
  stateMutex = xSemaphoreCreateMutex();
  if (!stateMutex) { Serial.println("Could not allocate state mutex"); while (true) delay(1000); }
  String saved = prefs.getString("stations", "");
  bool validSavedConfig = !saved.length() && !prefs.isKey("stations");
  if (saved.length()) {
    DynamicJsonDocument doc(kStationJsonCapacity);
    std::unique_ptr<StationConfig[]> restored(new (std::nothrow) StationConfig[kStations]);
    if (!restored || !doc.capacity()) return false;
    std::string error;
    if (!deserializeJson(doc, saved)) {
      const bool legacy = doc.is<JsonArray>();
      uint32_t counter = legacy ? 6 : doc["next_id"].as<uint32_t>();
      if (parseStationList(legacy ? doc.as<JsonVariantConst>() : doc["stations"].as<JsonVariantConst>(),
                           restored.get(), nullptr, counter, true, legacy, error)) {
        for (unsigned i = 0; i < kStations; ++i) configs[i] = restored[i];
        nextHueId = counter;
        validSavedConfig = true;
      } else Serial.println("Saved station configuration is invalid; please assign the stations again.");
    }
  }
  return validSavedConfig;
}

#include "rs_remote.h"

void startServices() {
  WiFi.setAutoReconnect(!usingLocalWifi());
  persistWifiCredentials();
  // Arduino ESP32 2.0.17 / IDF 4.4 requires modem sleep for Wi-Fi/BLE
  // coexistence. WIFI_PS_NONE aborts in coex_enable() when NimBLE starts.
  // Use the driver API: WiFi.setSleep() also returns false when unchanged.
  const esp_err_t powerSaveResult = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  if (powerSaveResult != ESP_OK) {
    Serial.printf("Wi-Fi modem sleep failed: %s; BLE will not start.\n",
                  esp_err_to_name(powerSaveResult));
  }
  if (powerSaveResult == ESP_OK) Serial.println("Wi-Fi ready; modem sleep policy enabled for Wi-Fi/BLE coexistence.");
  const bool validSavedConfig = loadStations();
  struct Services {
    bool validConfig, coexReady;
    bool reserveWorker() {
      if (!coexReady) { bleStartupError = "WIFI_SLEEP_FAILED"; return false; }
      const bool ok = xTaskCreatePinnedToCore(bleWorker,"lighthouse-ble",8192,nullptr,1,nullptr,0) == pdPASS;
      Serial.printf("BLE worker reservation: ok=%s, stack_bytes=8192, free=%u, largest=%u\n",
        ok ? "true" : "false",ESP.getFreeHeap(),ESP.getMaxAllocHeap());
      return ok;
    }
    void startWeb() { configureRoutes(); server.begin(); matterServicesReady = true; }
    bool startMatter() {
      if (!validConfig) { matterError = "SAVED_CONFIG_INVALID"; return false; }
      if (localWifiMode) return matterLocalBegin();
      if (!matterBeginResources()) return false;
      if (!matterBridgeBegin()) { matterEndResources(); return false; }
      const uint32_t started = millis();
      while (matterInitializing.load()) {
        // Avoid allocating page/TCP output buffers in the short model-build
        // peak. A slow/stalled CHIP task still exposes HTTP recovery.
        if (startupHttpDue(millis(),started)) server.handleClient();
        processSerial(); delay(2);
      }
      return matterReady.load();
    }
    bool startBle() {
      Serial.println("Initializing BLE ...");
      return NimBLEDevice::init("Lighthouse Bridge");
    }
    void failed(const char* code) {
      if (strcmp(bleStartupError.load(),"BLE_STARTING") == 0)
        bleStartupError = strcmp(code,"MATTER_START_FAILED") == 0 ? matterError.load() : code;
      if (!MatterBridgeInitializer::started() && MDNS.begin(hostname.c_str())) MDNS.addService("http","tcp",80);
      Serial.printf("BLE startup failed: error=%s, free=%u, largest=%u; HTTP recovery remains available.\n",
        bleStartupError.load(),ESP.getFreeHeap(),ESP.getMaxAllocHeap());
    }
    void releaseWorker() { bleStartupError = ""; bleWorkerAvailable = true; Serial.println("BLE worker released (read-only startup)."); }
  } services{validSavedConfig,powerSaveResult == ESP_OK};
  startMatterServices(services);
  alexaReady = matterReady.load() && bleWorkerAvailable.load();
  matterSetHostname(hostname);
  Serial.printf("Web interface: http://%s/ or http://%s.local/\n", bridgeAddress().toString().c_str(), hostname.c_str());
  servicesStarted = true;
  Serial.println(bleWorkerAvailable.load() ? "Lighthouse Bridge ready." : "HTTP recovery ready; Bluetooth unavailable.");
  Serial.printf("Memory ready: free_8bit=%u, largest_8bit=%u, minimum_8bit=%u, status_entry_bytes=%u\n",
    matterDataFree(),matterDataLargest(),heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
    static_cast<unsigned>(sizeof(StatusStation)));
  emitDeviceInfo();
}

void setup() {
  Serial.begin(115200);
  FactoryResetStorage resetStorage;
  if(!applyFactoryResetAtBoot(resetStorage)){
    Serial.println("FACTORY_RESET_STORAGE_FAILED");while(true)delay(1000);
  }
  if (!lh_rs_init_diagnostics()) Serial.println("RS_DIAGNOSTICS_INIT_FAILED");
  pinMode(LH_BOOT_PIN, INPUT_PULLUP);
  prefs.begin("lhbridge", false);
  roomName=prefs.getString("room_name","");
  if(roomName.length()&&!validRoomName(roomName.c_str()))roomName="";
  {
    const String savedButtons=prefs.getString("gpio_buttons","");
    if (savedButtons.length()) {
      StaticJsonDocument<384> doc;ButtonBinding restored[2];
      if (!deserializeJson(doc,savedButtons) && parseButtons(doc.as<JsonVariantConst>(),restored))
        for(unsigned i=0;i<2;++i) gpioButtons[i]=restored[i];
      else Serial.println("GPIO_BUTTON_CONFIG_INVALID: buttons disabled.");
    }
    applyButtonPins();
  }
  if (prefs.getBool("remote_mode", false)) runRemoteMode();
  delay(500);
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
  WiFi.persistent(true);
  localWifiMode = storedNetworkMode(prefs.getString("wifi_mode","home").c_str()) == NetworkMode::Local;
  if (!consumeWifiSetup(prefs,wifiSetupBoot)) Serial.println("WIFI_SETUP_FLAG_SAVE_FAILED");
  WiFi.onEvent([](arduino_event_id_t event,arduino_event_info_t info) {
    // Event context stores numbers only. All logging stays on the main loop.
    unsigned kind = 0;
    if (event == ARDUINO_EVENT_WIFI_AP_START) kind = 1;
    if (event == ARDUINO_EVENT_WIFI_AP_STOP) kind = 2;
    if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) { kind = 3; wifiApJoins.fetch_add(1); }
    if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
      kind = 4; wifiApLeaves.fetch_add(1); wifiApReason = info.wifi_ap_stadisconnected.reason;
    }
    if (kind) wifiApEvent.store(kind);
  });
  WiFi.mode(localWifiMode || wifiSetupBoot ? WIFI_AP : WIFI_STA);
  if (!localWifiMode && !wifiSetupBoot) WiFi.enableIPv6();
  accessKey = prefs.getString("access_key", "");
  if (accessKey.length() != 24) {
    accessKey = randomHex(12);
    if (prefs.putString("access_key", accessKey) != accessKey.length()) {
      Serial.println("NVS_KEY_SAVE_FAILED"); while (true) delay(1000);
    }
  }
  const String authentication = prefs.getString("web_auth", "");
  if (authentication.length()) {
    StaticJsonDocument<512> doc;
    if (!deserializeJson(doc, authentication)) {
      webSalt = doc["salt"].as<String>(); webHash = doc["hash"].as<String>();
      webUsernameEnabled = doc["username_enabled"].as<bool>();
      webUsername = storedWebUsername(doc.as<JsonVariantConst>());
      if (webSalt.length() != 32 || webHash.length() != 64) { webSalt = ""; webHash = ""; }
    }
  }
  // NetworkInterface::macAddress can return zeros before its netif exists.
  // Derive the same STA suffix independently of asynchronous Wi-Fi startup.
  uint8_t stationMac[6]{};
  char suffixBytes[7];
  if (esp_read_mac(stationMac, ESP_MAC_WIFI_STA) != ESP_OK) {
    Serial.println("DEVICE_MAC_READ_FAILED");
    restartBridge();
    return;
  }
  snprintf(suffixBytes, sizeof(suffixBytes), "%02X%02X%02X", stationMac[3], stationMac[4], stationMac[5]);
  String suffix = suffixBytes;
  apName = "Lighthouse-Bridge-Setup-" + suffix;
  hostname = "lighthouse-bridge-" + suffix;
  hostname.toLowerCase();
  customDeviceName = prefs.getString("dev_name", "");
  if (validDeviceName(customDeviceName.c_str())) hostname = apName = customDeviceName;
  else customDeviceName = "";
  WiFi.setHostname(hostname.c_str());
  Serial.printf("\nLH-Bridge %s\n", LH_FIRMWARE_VERSION);
  Serial.println("Setup Wi-Fi / API key: " + accessKey);
  Serial.println("Web login: first use the key, then your own password; username is optional.");
  emitDeviceInfo();
  wifiManager.setDebugOutput(false);
  wifiManager.setTitle("Lighthouse Bridge");
  wifiManager.setShowInfoUpdate(false);
  // Keep firmware updates on the authenticated main interface. First-match
  // handlers precede WiFiManager's built-in raw-BIN updater in the pinned library.
  wifiManager.setWebServerCallback([] {
    auto denied = [] { wifiManager.server->send(403, "text/plain", "Use the signed-in Bridge firmware update page."); };
    wifiManager.server->on("/update", HTTP_ANY, denied, [] {});
    wifiManager.server->on("/u", HTTP_ANY, denied, [] {});
    wifiManager.server->on("/",HTTP_GET,[] {
      if (!wifiManager.beginCustomRoot()) return;
      String page = kRsWifiPortalPage;
      page.replace("%PORTAL_HEAD%",kRsWifiPortalHead);
      String name = apName; name.replace("&","&amp;"); name.replace("<","&lt;"); name.replace(">","&gt;");
      page.replace("%SSID%",name);
      wifiManager.server->sendHeader("Cache-Control","no-store");
      wifiManager.server->sendHeader("X-Frame-Options","DENY");
      wifiManager.server->sendHeader("X-Content-Type-Options","nosniff");
      wifiManager.server->send(200,"text/html; charset=utf-8",page);
    });
    portalModeToken = randomHex(16);
    const char* portalHeaders[] = {"Origin"};
    wifiManager.server->collectHeaders(portalHeaders,1);
    wifiManager.server->on("/local", HTTP_GET, [] {
      String page = kLocalWifiPage; page.replace("%PORTAL_HEAD%",kRsWifiPortalHead); page.replace("%TOKEN%",portalModeToken);
      wifiManager.server->sendHeader("Cache-Control","no-store");
      wifiManager.server->sendHeader("X-Frame-Options","DENY");
      wifiManager.server->send(200,"text/html; charset=utf-8",page);
    });
    wifiManager.server->on("/local", HTTP_POST, [] {
      auto& portal = *wifiManager.server;
      const String origin = portal.header("Origin");
      if (!portalModeToken.length() || !equalSecret(portal.arg("token"),portalModeToken) ||
          (origin.length() && origin != "http://" + portal.hostHeader())) {
        portal.send(403,"text/plain","Request denied"); return;
      }
      if (!saveNetworkMode(prefs,NetworkMode::Local)) { portal.send(500,"text/plain","Save failed"); return; }
      portalModeToken = ""; // Consume the choice once; restart happens outside the handler.
      portal.sendHeader("Cache-Control","no-store");
      String page = kLocalWifiSaved; page.replace("%PORTAL_HEAD%",kRsWifiPortalHead);
      portal.send(200,"text/html; charset=utf-8",page);
      restartAt = millis(); restartPending = true;
    });
  });
  const char* portalMenu[] = {"wifi","custom","info","exit"};
  wifiManager.setMenu(portalMenu,4);
  wifiManager.setCustomMenuHTML(kLocalWifiMenu);
  wifiManager.setHostname(hostname);
  wifiManager.setCustomHeadElement("<meta name='color-scheme' content='dark'><style>"
    "body{margin:0;background:#101817;color:#edf5ef;font:16px/1.5 system-ui,sans-serif}"
    ".wrap{display:block;box-sizing:border-box;min-width:0;width:calc(100% - 32px);max-width:560px;margin:32px auto;padding:24px;background:#1b2624;border:1px solid #33433f;border-radius:16px}"
    "h1{letter-spacing:-1px}a{color:#c2f78c}a:hover{color:#edf5ef}.q{filter:invert(1)}button,input,select{box-sizing:border-box;font:inherit;border-radius:10px;padding:12px}"
    "button{background:#c2f78c;color:#15210f;border:1px solid #c2f78c;font-weight:600}"
    "input,select{background:#101817;color:#edf5ef;border:1px solid #33433f}.msg{background:#1b2624;border-radius:10px}"
    "input:focus,button:focus{outline:2px solid #c2f78c;outline-offset:2px}</style>");
  wifiManager.setCustomHeadElement(kRsWifiPortalHead);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConnectTimeout(20);
  wifiManager.setConfigPortalTimeout(0);
  wifiManager.setAPCallback([](WiFiManager*) { emitWifiInfo(); });
  wifiManager.setSaveConfigCallback([] {
    persistWifiCredentials();
    if (WiFi.status() == WL_CONNECTED && !saveNetworkMode(prefs,NetworkMode::Home))
      Serial.println("WIFI_MODE_SAVE_FAILED");
  });
  if (wifiSetupBoot) {
    // No Matter/BLE services have been initialized on this fresh boot.
    WiFi.setAutoReconnect(false); manualPortal = true;
    wifiManager.setConfigPortalTimeout(180);
    Serial.println("Clean Wi-Fi setup: AP-only startup before Matter/BLE.");
    wifiManager.startConfigPortal(apName.c_str(),accessKey.c_str());
    return;
  }
  if (localWifiMode) {
    WiFi.setAutoReconnect(false);
    localApReady = WiFi.softAPConfig(IPAddress(192,168,4,1),IPAddress(192,168,4,1),IPAddress(255,255,255,0)) &&
      WiFi.softAP(apName.c_str(),accessKey.c_str(),1,false,4);
    if (localApReady) { Serial.println("Local Wi-Fi ready: http://192.168.4.1/"); emitWifiInfo(); return; }
    Serial.println("LOCAL_WIFI_START_FAILED; opening setup recovery.");
    localWifiMode = false; // Saved choice stays intact; the setup portal remains recovery.
    manualPortal = true; // Saving/closing recovery must reboot out of the original AP-only mode.
    wifiManager.startConfigPortal(apName.c_str(),accessKey.c_str());
    return;
  }
  wifi_config_t savedWifi{};
  const bool firstSetup = esp_wifi_get_config(WIFI_IF_STA, &savedWifi) == ESP_OK && !savedWifi.sta.ssid[0];
  memset(&savedWifi, 0, sizeof(savedWifi));
  // With no saved network, switch directly from STA to AP. autoConnect()
  // otherwise lets WiFiManager turn off the only active interface before
  // starting AP, which stops and reinitializes the Wi-Fi driver.
  if (firstSetup && WiFi.mode(WIFI_AP))
    wifiManager.startConfigPortal(apName.c_str(), accessKey.c_str());
  else
    wifiManager.autoConnect(apName.c_str(), accessKey.c_str());
  if (wifiManager.getConfigPortalActive()) emitWifiInfo();
  // The main loop services USB info while the setup portal waits for Wi-Fi.
}

void loop() {
  processSerial();
  const unsigned apEvent = wifiApEvent.exchange(0);
  if (apEvent) {
    Serial.printf("Wi-Fi AP event: kind=%u, joins=%u, leaves=%u, last_reason=%u\n",
      apEvent,wifiApJoins.load(),wifiApLeaves.load(),wifiApReason.load());
  }
  wifiManager.process();
  if (restartPending && millis() - restartAt >= 1500
      && maintenanceRestartReady(millis(),restartAt,maintenanceWaitIdle,bleActive)
     ) restartBridge();
  if (otaPrepared && !otaUploading && !otaSucceeded && millis() - otaPreparedAt >= 60000) cancelOta("OTA_ABORTED");
  if (manualPortal) {
    if (!wifiManager.getConfigPortalActive()) {
      if (WiFi.status() == WL_CONNECTED) persistWifiCredentials();
      restartBridge();
    }
    delay(2); return;
  }
  if (!servicesStarted) {
    if (usingLocalWifi() || WiFi.status() == WL_CONNECTED) {
      if (wifiManager.getConfigPortalActive()) wifiManager.stopConfigPortal();
      startServices();
    }
    delay(2); return;
  }
  server.handleClient();
  matterBridgeLoop();
  alexaReady = matterReady.load() && bleWorkerAvailable.load();
  for (unsigned i=0;i<2;++i) {
    const auto& binding=gpioButtons[i];
    if (binding.pin>=0 && gpioDebounce[i].poll(digitalRead(binding.pin)!=LOW,millis())) {
      const bool accepted=queueTarget(-1,binding.action==ButtonAction::AllOn?Target::On:Target::Sleep,
                                     binding.action==ButtonAction::AllOff);
      Serial.printf("GPIO button: index=%u, accepted=%s\n",i+1,accepted?"true":"false");
    }
  }
  static uint32_t pressedAt = 0;
  if (digitalRead(LH_BOOT_PIN) == LOW) {
    if (!pressedAt) pressedAt = millis();
    if (millis() - pressedAt >= 5000) {
      Lock lock;
      if (!otaPrepared && !occupied()) {
        if (!portalRequested && !restartPending && requestWifiSetup(prefs)) { blePaused = true; portalRequested = true; }
        else if (!portalRequested && !restartPending) Serial.println("WIFI_SETUP_FLAG_SAVE_FAILED");
      }
    }
  } else pressedAt = 0;
  if (portalRequested && !bleActive) {
    portalRequested = false;
    // Reboot frees the initialized radio/protocol heaps before opening setup.
    restartAt = millis(); restartPending = true;
  }
  delay(2);
}
