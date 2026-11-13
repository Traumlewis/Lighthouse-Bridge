#include <Arduino.h>
#include <ArduinoJson.h>
#ifdef LH_MATTER_BUILD
#include <Matter.h>
#else
#include <Espalexa.h>
#endif
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
#ifndef LH_MATTER_BUILD
#include "hue_api.h"
#include "web_ui.h"
#else
#include "web_ui_gzip.h"
#include "status_api.h"
#include "startup_sequence.h"
#include "config_stream.h"
#include "matter_http.h"
#include "http_body.h"
#include <esp_mac.h>
#include <esp_heap_caps.h>
#include <atomic>
#endif
#include "password_hash.h"

#ifndef LH_FIRMWARE_VERSION
#define LH_FIRMWARE_VERSION "0.3.9"
#endif
#if MBEDTLS_VERSION_MAJOR >= 3
#define LH_SHA_START mbedtls_sha256_starts
#define LH_SHA_UPDATE mbedtls_sha256_update
#define LH_SHA_FINISH mbedtls_sha256_finish
#else
#define LH_SHA_START mbedtls_sha256_starts_ret
#define LH_SHA_UPDATE mbedtls_sha256_update_ret
#define LH_SHA_FINISH mbedtls_sha256_finish_ret
#endif

using namespace bridge;
#ifdef LH_MATTER_BUILD
MatterWebServer server(80);
static_assert(CONFIG_BT_NIMBLE_MAX_CONNECTIONS == 1,"The sequential Matter worker needs one BLE connection");
static_assert(!CONFIG_BT_NIMBLE_ROLE_PERIPHERAL && !CONFIG_BT_NIMBLE_ROLE_BROADCASTER &&
              CONFIG_BT_NIMBLE_ROLE_CENTRAL && CONFIG_BT_NIMBLE_ROLE_OBSERVER,"Use only the Lighthouse client/scanner roles");
#else
WebServer server(80);
#endif
#ifndef LH_MATTER_BUILD
Espalexa alexa;
#endif
Preferences prefs;
SemaphoreHandle_t stateMutex;
StationConfig configs[kStations];
StationState states[kStations];
#ifdef LH_MATTER_BUILD
std::atomic<bool> bleWorkerAvailable{false};
std::atomic<const char*> bleStartupError{"BLE_STARTING"};
#endif
uint32_t nextHueId = 1;
String customDeviceName;
bool otaPrepared = false, otaUploading = false, otaSucceeded = false, otaCandidateReady = false, otaRequestAuthorized = false;
void emitDeviceInfo();
String otaToken, otaOwner, otaError;
uint32_t otaPreparedAt = 0, restartAt = 0;
bool restartPending = false;
String accessKey, apName, hostname;
String webSalt, webHash;
struct WebSession { String token; uint32_t seen = 0; bool bootstrap = false; };
WebSession sessions[4];
unsigned loginFailures = 0;
uint32_t loginBlockedAt = 0;
bool loginBlocked = false;
WiFiManager wifiManager;
bool servicesStarted = false, manualPortal = false;
bool blePaused = false;
bool bleActive = false;
bool portalRequested = false;
bool alexaReady = false;
bool scanPending = false, scanning = false;
bool refreshRequested[kStations]{};
uint32_t scanFinishedMs = 0;
#ifdef LH_MATTER_BUILD
using FoundStation = StatusDiscovery;
#else
struct FoundStation { char name[48]; char mac[18]; uint8_t type; int rssi; };
#endif
FoundStation found[16]{};
#ifdef LH_MATTER_BUILD
#include "matter_scan.h"
#endif
unsigned foundCount = 0;
bool foundTruncated = false;

struct Lock {
  Lock() { xSemaphoreTake(stateMutex, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(stateMutex); }
};

#ifdef LH_MATTER_BUILD
uint32_t matterDataFree() { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); }
uint32_t matterDataLargest() { return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT); }
struct JsonHttpSink {
  size_t write(const uint8_t* data, size_t size) {
    return server.writeBytes(data,size);
  }
};
#endif
void sendJson(int code, const JsonDocument& doc) {
  if (doc.overflowed()) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(500, "application/json", "{\"error\":\"OUT_OF_MEMORY\"}");
    return;
  }
  server.sendHeader("Cache-Control", "no-store");
#ifdef LH_MATTER_BUILD
  server.setContentLength(measureJson(doc));
  server.send(code,"application/json; charset=utf-8","");
  JsonHttpSink sink; JsonStreamWriter writer(sink);
  serializeJson(doc,writer);
  if (!writer.flush()) server.client().stop();
#else
  String body; serializeJson(doc,body);
  server.send(code,"application/json; charset=utf-8",body);
#endif
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
  if (scanning || scanPending) return true;
  if (includeRead && bleActive) return true;
  for (auto& state : states) if (state.busy || state.pending || (includeRead && state.reading)) return true;
  return false;
}
bool queueTarget(int slot, Target target) {
#ifdef LH_MATTER_BUILD
  if (!bleWorkerAvailable.load()) return false;
#endif
  Lock lock;
  bool accepted = false;
  if (blePaused || portalRequested || otaPrepared || restartPending) return false;
  for (unsigned i = 0; i < kStations; ++i) {
    if ((slot < 0 || slot == static_cast<int>(configs[i].id)) && configs[i].mac[0]) {
      request(states[i], target, millis());
      accepted = true;
    }
  }
  return accepted;
}

#ifndef LH_MATTER_BUILD
bool queueHueTarget(int hueId, Target target) {
  const int stationId = stationIdForHue(configs, static_cast<uint32_t>(hueId));
  return stationId && queueTarget(stationId, target);
}
#endif

// BLE is confined to this worker: scan, connect, write and disconnect never
// block the HTTP/SSDP loop. One connection at a time for up to 16 stations.
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
#ifdef LH_MATTER_BUILD
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
#else
  // A failed establishment (HCI 0x3e) needs a new radio opportunity, not a
  // longer GATT read timeout. Passive scanning avoids extra scan requests.
  auto* scan = NimBLEDevice::getScan();
  scan->setActiveScan(false);
  scan->setInterval(97);
  scan->setWindow(67);
  auto results = scan->getResults(2000, false); // NimBLE 2.x durations are milliseconds.
  bool seen = false;
  bool matched = false;
  for (int i = 0; i < results.getCount(); ++i) {
    const auto* device = results.getDevice(i);
    char mac[18]{};
    normalizeMac(device->getAddress().toString().c_str(), mac);
    if (strcmp(mac, config.mac)) continue;
    matched = true;
    const uint8_t eventType = device->getAdvType();
    // NimBLE 1.4.3 isConnectable() treats legacy enum 0 (ADV_IND) as a
    // zero bitmask on classic ESP32, incorrectly rejecting normal advertising.
    static_assert(BLE_HCI_ADV_RPT_EVTYPE_ADV_IND == 0 && BLE_HCI_ADV_RPT_EVTYPE_DIR_IND == 1,
                  "Legacy advertising classification must match HCI constants");
    if (!isConnectableLegacyAdvertisement(eventType)) {
      Serial.printf("Station %u: retry scan: address seen, RSSI=%d dBm, adv_type=%u (not connectable)\n",
                    config.id, device->getRSSI(), eventType);
      continue;
    }
    // Random (1) is fixed for Lighthouse V2. Advertising confirms visibility;
    // never substitute another device based on its name.
    config.addressType = 1;
    Serial.printf("Station %u: retry scan: advertisement seen, RSSI=%d dBm, address_type=%u, advertised_address_type=%u, adv_type=%u\n",
                  config.id, device->getRSSI(), config.addressType, device->getAddress().getType(), eventType);
    seen = true;
    break;
  }
  if (!seen) Serial.printf("Station %u: retry scan: %s within 2 seconds; devices_seen=%d\n", config.id,
                          matched ? "address seen without connectable advertisement" : "assigned address not seen",
                          results.getCount());
  scan->clearResults();
#endif
}

void logBleFailure(unsigned slot, const char* error, int code, unsigned attempt, uint8_t addressType) {
  Serial.printf("Station %u: %s, BLE code=%d, worker_attempt=%u, address_type=%u, host_retry_limit=1%s\n", slot,
                error, code, attempt + 1, addressType,
                code == BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT)
                  ? " (HCI 0x3e: connection establishment failed)"
                  : code == BLE_HS_ETIMEOUT ? " (connection timeout)" : "");
}

bool accessPower(const StationConfig& config, const Target* target, int& observed, char* error, int& bleError) {
  waitForBleCleanup();
  bleError = 0;
#ifdef LH_MATTER_BUILD
  if (matterDataFree() < 4096 || matterDataLargest() < sizeof(NimBLEClient)+128) {
    strcpy(error,"BLE_NO_MEMORY"); bleError = BLE_HS_ENOMEM; return false;
  }
#endif
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
    auto* power = service ? service->getCharacteristic(kPower) : nullptr;
    if (!power || (target && !power->canWriteNoResponse() && !power->canWrite())) {
      strcpy(error, "POWER_CHARACTERISTIC_MISSING");
    } else {
      ok = !target;
      if (target) {
        const uint8_t command = static_cast<uint8_t>(*target); // Only 0x00 / 0x01.
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
#ifdef LH_MATTER_BUILD
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
#else
  auto* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(97);
  scan->setWindow(67);
  auto results = scan->getResults(5000, false);
  FoundStation next[16]{};
  unsigned count = 0;
  bool truncated = false;
  for (int i = 0; i < results.getCount(); ++i) {
    const auto* device = results.getDevice(i);
    const std::string name = device->getName();
    if (name.compare(0, 4, "LHB-") != 0) continue;
    if (count == 16) { truncated = true; continue; }
    auto& item = next[count++];
    strncpy(item.name, name.c_str(), sizeof(item.name) - 1);
    normalizeMac(device->getAddress().toString().c_str(), item.mac);
    item.type = device->getAddress().getType();
    item.rssi = device->getRSSI();
  }
  scan->clearResults();
#endif
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
#ifdef LH_MATTER_BUILD
  while (!bleWorkerAvailable.load()) vTaskDelay(pdMS_TO_TICKS(20));
#endif
  unsigned cursor = 0;
  for (;;) {
    waitForBleCleanup();
    int selected = -1;
    StationConfig config;
    Target target = Target::Sleep;
    uint32_t generation = 0;
    bool doScan = false;
    bool readOnly = false;
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
      if (selected < 0 && scanPending) {
        scanPending = false;
        scanning = doScan = true;
      }
      if (selected < 0 && !doScan) {
        for (unsigned offset = 0; offset < kStations; ++offset) {
          const unsigned i = (cursor + offset) % kStations;
          const auto& state = states[i];
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
    { Lock lock; bleActive = NimBLEDevice::getCreatedClientCount() != 0; }
    vTaskDelay(pdMS_TO_TICKS(selected >= 0 ? 150 : 20));
  }
}

#ifdef LH_MATTER_BUILD
#include "matter_config_io.h"
#include "matter_adapter.h"
#include "matter_status.h"
#include "matter_config_route.h"
#endif

void statusRoute() {
#ifdef LH_MATTER_BUILD
  matterStatusRoute();
#else
  if (!authenticated()) return;
  DynamicJsonDocument doc(32768);
  doc["version"] = LH_FIRMWARE_VERSION;

  doc["poll_interval_ms"] = 15000;
  doc["max_stations"] = kStations;
  doc["ota_supported"] = esp_ota_get_next_update_partition(nullptr) != nullptr;
  doc["ota_product"] = kOtaProduct;
  doc["setup_ssid"] = apName;
  doc["maintenance"] = blePaused || portalRequested || otaPrepared;
  doc["ip"] = WiFi.localIP().toString();
  doc["hostname"] = hostname;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["alexa_started"] = alexaReady;
  doc["free_heap"] = ESP.getFreeHeap();
  {
    Lock lock;
    const uint32_t now = millis();
    doc["uptime_ms"] = now;
    doc["scanning"] = scanning || scanPending;
    doc["scan_finished_ms"] = scanFinishedMs;
    doc["scan_truncated"] = foundTruncated;
    auto stations = doc.createNestedArray("stations");
    for (unsigned i = 0; i < kStations; ++i) {
      if (!configs[i].id) continue;
      auto item = stations.createNestedObject();
      item["id"] = configs[i].id;
      item["name"] = configs[i].name;
      item["mac"] = configs[i].mac;
      item["address_type"] = configs[i].addressType;
      item["alexa_compat"] = configs[i].alexaCompat;
      item["configured"] = configs[i].mac[0] != '\0';
      const auto& state = states[i];
      if (state.hasRequest) item["requested"] = targetName(state.requested);
      else item["requested"] = nullptr;
      item["pending"] = state.pending;
      item["busy"] = state.busy;
      item["reading"] = state.reading || refreshRequested[i];
      item["read_active"] = state.reading;
      item["read_queued"] = refreshRequested[i];
      item["reachable"] = freshMeasurement(state, now);
      item["read_failures"] = state.readFailures;
      item["ble_error"] = state.bleError;
      item["last_read_attempt_ms"] = state.lastReadAttemptMs;
      item["requested_ms"] = state.requestedMs;
      item["alexa_target_pending"] = hueTargetPending(state, now);
      item["command_sent"] = state.sent;
      item["command_confirmed"] = state.commandConfirmed;
      item["command_status"] = commandStatus(state, now);
      item["last_sent_ms"] = state.lastSentMs;
      item["observed"] = observedName(state.observed);
      item["observed_raw"] = state.observed;
      item["observed_ms"] = state.observedMs;
      item["error"] = state.error;
    }
    auto devices = doc.createNestedArray("discovered");
    for (unsigned i = 0; i < foundCount; ++i) {
      auto item = devices.createNestedObject();
      item["name"] = found[i].name;
      item["mac"] = found[i].mac;
      item["address_type"] = found[i].type;
      item["rssi"] = found[i].rssi;
    }
  }
  sendJson(200, doc);
#endif
}

#ifndef LH_MATTER_BUILD
// Hue ACK means queued. GET ON temporarily follows an active accepted target;
// measurements and reachability remain separate and never become optimistic.
// Espalexa still owns SSDP, description and username registration.
bool serveHueLights(const String& uri) {
  const uint32_t started = millis();
  String mac = WiFi.macAddress(), suffix = mac; suffix.replace(":", "");
  const uint32_t mac24 = strtoul(suffix.substring(6).c_str(), nullptr, 16);
  const auto route = parseHueLightRoute(uri.c_str(), mac24);
  if (!route.matched) return false;
  if (server.method() == HTTP_PUT) {
    const int selected = route.slot < 0 ? -1 : stationHueIndex(configs,route.slot);
    const bool compat = selected >= 0 && configs[selected].alexaCompat;
    const auto reply = handleHueWrite(route, server.arg("plain").c_str(), queueHueTarget, compat);
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", reply.json.c_str());
    Serial.printf("Hue PUT: station=%d, on=%s, queued=%s, unsupported=%u, handler_ms=%u\n",
                  stationIdForHue(configs, route.slot),
                  reply.on ? "true" : "false", reply.accepted ? "true" : "false", reply.unsupported,
                  millis() - started);
    return true;
  }
  if (server.method() != HTTP_GET || !route.valid) {
    const auto error = hueError(route.valid ? 4 : 3, "/lights",
                               route.valid ? "method not available" : "resource not available");
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", error.c_str());
    return true;
  }
  const int selected = route.slot < 0 ? -1 : stationHueIndex(configs, route.slot);
  if (route.slot >= 0 && (selected < 0 || !configs[selected].mac[0])) {
    const auto error = hueError(3, "/lights", "resource not available");
    server.send(200, "application/json", error.c_str()); return true;
  }
  const bool stateOnly = route.stateOnly;
  unsigned configured = 0, fresh = 0;
  unsigned traceBri = 0;
  bool traceOn = false, traceReachable = false, tracePending = false, traceTarget = false;
  DynamicJsonDocument doc(16384);
  JsonObject root = doc.to<JsonObject>();
  {
    Lock lock;
    const uint32_t now = millis();
    for (unsigned i = 0; i < kStations; ++i) {
      if (!configs[i].id || !configs[i].mac[0]) continue;
      ++configured;
      if (freshMeasurement(states[i], now)) ++fresh;
      if (!states[i].reading && !states[i].busy &&
          now - states[i].lastReadAttemptMs >= readInterval(states[i], now)) refreshRequested[i] = true;
      if (selected >= 0 && selected != static_cast<int>(i)) continue;
      JsonObject light = selected < 0 ? root.createNestedObject(String(hueKey(mac24, configs[i].hueId))) : root;
      writeHueLight(light, configs[i], states[i], now, mac.c_str(), "lighthouse-bridge-0.3.9", stateOnly);
      JsonObject state = stateOnly ? light : light["state"].as<JsonObject>();
      traceOn = state["on"].as<bool>(); traceBri = state["bri"].as<unsigned>(); traceReachable = state["reachable"].as<bool>();
      tracePending |= states[i].pending || states[i].busy;
      traceTarget |= hueTargetPending(states[i], now);
    }
  }
  sendJson(200, doc);
  if (selected < 0) Serial.printf("Hue GET lights: configured=%u, fresh=%u, pending=%s, source=%s\n",
      configured, fresh, tracePending ? "true" : "false", traceTarget ? "accepted_target" : "measurement");
  else Serial.printf("Hue GET: station=%d, profile=%s, on=%s, bri=%u, reachable=%s, pending=%s, source=%s\n",
      configs[selected].id, configs[selected].alexaCompat ? "compat" : "standard", traceOn ? "true" : "false", traceBri, traceReachable ? "true" : "false",
      tracePending ? "true" : "false", traceTarget ? "accepted_target" : "measurement");
  return true;
}

#endif

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

void configureMaintenanceRoutes() {
  server.on("/api/device", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    if (otaPrepared || blePaused) { fail(409, "BLE_BUSY"); return; }
    StaticJsonDocument<256> doc;
    if (!readBody(doc)) return;
    if (!doc["name"].is<const char*>() || !validDeviceName(doc["name"])) { fail(400, "DEVICE_NAME_INVALID"); return; }
    const String name = doc["name"].as<String>();
    if (prefs.putString("dev_name", name) != name.length()) { fail(500, "SAVE_FAILED"); return; }
    customDeviceName = hostname = apName = name;
    WiFi.setHostname(hostname.c_str()); wifiManager.setHostname(hostname);
#ifdef LH_MATTER_BUILD
    matterSetHostname(hostname);
#else
    MDNS.end(); MDNS.begin(hostname.c_str()); MDNS.addService("http", "tcp", 80);
#endif
    doc.clear(); doc["saved"] = true; doc["hostname"] = hostname; doc["setup_ssid"] = apName;
    doc["url"] = "http://" + hostname + ".local/"; sendJson(200, doc); emitDeviceInfo();
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
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.sendHeader("X-Frame-Options", "DENY");
#ifdef LH_MATTER_BUILD
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
#else
    server.send_P(200, "text/html; charset=utf-8", WEB_UI);
#endif
  });
  server.on("/api/login", HTTP_POST, [] {
    if (!sameOriginJson()) return;
    StaticJsonDocument<512> doc;
    if (!readBody(doc)) return;
    if (loginBlocked && millis() - loginBlockedAt < 30000) { fail(429, "LOGIN_THROTTLED"); return; }
    if (loginBlocked) { loginBlocked = false; loginFailures = 0; }
    const String password = doc["password"].as<String>();
    const bool bootstrap = !webHash.length();
    const bool valid = doc["password"].is<const char*>() && password.length() <= 128 &&
      (bootstrap ? equalSecret(password, accessKey) : equalSecret(passwordHash(password, webSalt), webHash));
    if (!valid) {
      if (++loginFailures >= 5) { loginBlocked = true; loginBlockedAt = millis(); }
      fail(401, "LOGIN_FAILED"); return;
    }
    loginFailures = 0;
    issueSession(bootstrap);
    doc.clear(); doc["password_required"] = bootstrap; sendJson(200, doc);
  });
  server.on("/api/account", HTTP_GET, [] {
    if (!authenticated(true, true)) return;
    StaticJsonDocument<512> doc;
#ifdef LH_MATTER_BUILD
    doc["alexa_protocol"] = "matter"; doc["matter_ready"] = matterReady.load() && bleWorkerAvailable.load();
#endif
    doc["password_required"] = !webHash.length();
    doc["setup_ssid"] = apName; doc["hostname"] = hostname;
    doc["device_name"] = customDeviceName;
    if (webHash.length()) doc["api_key"] = accessKey;
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
#ifdef LH_MATTER_BUILD
    if (!bleWorkerAvailable.load()) { fail(503,bleStartupError.load()); return; }
#endif
    { Lock lock; for (unsigned i = 0; i < kStations; ++i) if (configs[i].mac[0]) refreshRequested[i] = true; }
    StaticJsonDocument<64> doc; doc["accepted"] = true; sendJson(202, doc);
  });
  server.on("/api/status", HTTP_GET, statusRoute);
  server.on("/api/power", HTTP_POST, [] {
    if (!mutationAllowed()) return;
#ifdef LH_MATTER_BUILD
    if (!bleWorkerAvailable.load()) { fail(503,bleStartupError.load()); return; }
#endif
    StaticJsonDocument<256> doc;
    if (!readBody(doc)) return;
    Target target;
    if (!doc["state"].is<const char*>() || !parseTarget(doc["state"], target)) {
      fail(400, "STATE_INVALID"); return;
    }
    int slot = -1;
    if (doc["station"].is<int>() && doc["station"].as<int>() >= 1 && doc["station"].as<int>() <= static_cast<int>(kStations))
      slot = doc["station"].as<int>();
    else if (!doc["station"].is<const char*>() || strcmp(doc["station"], "all") != 0) {
      fail(400, "STATION_INVALID"); return;
    }
    if (!queueTarget(slot, target)) { fail(409, "NO_STATION"); return; }
    doc.clear(); doc["accepted"] = true;
    sendJson(202, doc);
  });
  server.on("/api/scan", HTTP_POST, [] {
    if (!mutationAllowed()) return;
#ifdef LH_MATTER_BUILD
    if (!bleWorkerAvailable.load()) { fail(503,bleStartupError.load()); return; }
#endif
    bool queued = false;
    { Lock lock; if (blePaused || otaPrepared) { fail(409, "BLE_BUSY"); return; } if (!scanning) { queued = !scanPending; scanPending = true; } }
    if (queued) Serial.println("BLE discovery: queued (read-only).");
    StaticJsonDocument<64> doc; doc["accepted"] = true; sendJson(202, doc);
  });
  server.on("/api/config", HTTP_POST, [] {
#ifdef LH_MATTER_BUILD
    matterSaveConfiguration();
#else
    if (!mutationAllowed()) return;
    if (blePaused || otaPrepared) { fail(409, "BLE_BUSY"); return; }
#ifdef LH_MATTER_BUILD
    DynamicJsonDocument doc(kStationJsonCapacity);
#else
    DynamicJsonDocument doc(16384);
#endif
    if (!readBody(doc)) return;
    std::unique_ptr<StationConfig[]> next(new (std::nothrow) StationConfig[kStations]);
    std::unique_ptr<StationState[]> restored(new (std::nothrow) StationState[kStations]);
    if (!next || !restored) { fail(500, "SAVE_FAILED"); return; }
    uint32_t counter = nextHueId;
    std::string error;
    if (!parseStationList(doc["stations"], next.get(), configs, counter, false, false, error)) {
      fail(400, error.c_str()); return;
    }
    doc.clear(); doc["next_id"] = counter;
    writeStationList(doc.createNestedArray("stations"), next.get(), true);
    if (doc.overflowed()) { fail(503,"OUT_OF_MEMORY"); return; }
    String saved; serializeJson(doc, saved);
    bool busy = false, persisted = false;
#ifdef LH_MATTER_BUILD
    std::string matterConfigError;
#endif
    {
      Lock lock;
      busy = occupied(false);
      if (!busy) {
#ifdef LH_MATTER_BUILD
        if (matterPrepareConfiguration(next.get(), matterConfigError))
#endif
        persisted = prefs.putString("stations", saved) == saved.length();
        if (persisted) {
#ifdef LH_MATTER_BUILD
          blePaused = true; matterReady = false;
#endif
          reconcileStationStates(next.get(), configs, states, restored.get());
          for (unsigned i = 0; i < kStations; ++i) {
            configs[i] = next[i]; states[i] = restored[i]; refreshRequested[i] = false;
          }
          nextHueId = counter;
        }
      }
    }
    if (busy) { fail(409, "BLE_BUSY"); return; }
    if (!persisted) {
#ifdef LH_MATTER_BUILD
      if (!matterConfigError.empty()) { fail(409, matterConfigError.c_str()); return; }
#endif
      fail(500, "SAVE_FAILED"); return;
    }
#ifdef LH_MATTER_BUILD
    matterConfigurationChanged();
#endif
    doc.clear(); doc["saved"] = true; writeStationList(doc.createNestedArray("stations"), configs);
    sendJson(200, doc);
#endif
  });
  server.on("/api/wifi", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    { Lock lock; if (occupied(false) || otaPrepared) { fail(409, "BLE_BUSY"); return; } blePaused = true; portalRequested = true; }
    StaticJsonDocument<96> doc; doc["setup_ssid"] = apName; doc["accepted"] = true;
    sendJson(202, doc);
  });
  // Espalexa needs its Hue-compatible endpoints without our admin login.
#ifdef LH_MATTER_BUILD
  server.on("/api/matter", HTTP_GET, [] {
    if (!authenticated(true)) return;
    StaticJsonDocument<1024> doc; matterPrivateInfo(doc, true); sendJson(200, doc);
  });
  server.on("/api/matter/commissioning", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    if (!matterOpenCommissioning()) { fail(409, "MATTER_NOT_READY"); return; }
    StaticJsonDocument<64> doc; doc["accepted"] = true; sendJson(202, doc);
  });
#endif
  server.onNotFound([] {
    const String uri = server.uri();
    if (uri == "/api/status" || uri == "/api/power" || uri == "/api/config" ||
        uri == "/api/scan" || uri == "/api/wifi" || uri == "/api/refresh" ||
        uri == "/api/login" || uri == "/api/logout" || uri == "/api/password" || uri == "/api/account" ||
        uri == "/api/device" || uri == "/api/ota" || uri == "/api/ota/prepare" || uri == "/api/ota/cancel" ||
        uri == "/api/matter" || uri == "/api/matter/commissioning") {
      if (authenticated()) fail(404, "NOT_FOUND");
      return;
    }
#ifndef LH_MATTER_BUILD
    // Hue also uses /api/<username>/lights. Do not intercept it as our API.
    if (uri == "/api" || uri.startsWith("/api/")) {
      if (server.header("Origin").length()) { fail(403, "ORIGIN_DENIED"); return; }
      if (serveHueLights(uri)) return;
      // Only username registration may reach the vendor's write parser.
      // Its substring matching must never bypass strict light validation.
      if (uri == "/api" && server.method() == HTTP_POST &&
          alexa.handleAlexaApiCall(uri, server.arg("plain"))) return;
      if (server.method() == HTTP_GET && alexa.handleAlexaApiCall(uri, "")) return;
    }
#endif
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
  if (WiFi.status() == WL_CONNECTED) doc["url"] = "http://" + WiFi.localIP().toString() + "/";
  else doc["url"] = nullptr;
  Serial.print("LH_INFO "); serializeJson(doc, Serial); Serial.println();
}

void processSerial() {
  static char line[80]; static unsigned length = 0; static bool overflow = false;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line[length] = 0;
      if (!overflow && strcmp(line, "LH_INFO") == 0) emitDeviceInfo();
#ifdef LH_MATTER_BUILD
      if (!overflow && strcmp(line, "LH_MATTER_INFO") == 0) emitMatterInfo();
#endif
      if (!overflow && strcmp(line, "LH_RESET_WEB_PASSWORD") == 0) {
        prefs.remove("web_auth"); webSalt = ""; webHash = "";
        for (auto& session : sessions) session.token = "";
        Serial.println("LH_PASSWORD_RESET");
      }
      length = 0; overflow = false;
    } else if (length < sizeof(line) - 1 && !overflow) line[length++] = c;
    else overflow = true;
  }
}

void startServices() {
  WiFi.setAutoReconnect(true);
  persistWifiCredentials();
  // Arduino ESP32 2.0.17 / IDF 4.4 requires modem sleep for Wi-Fi/BLE
  // coexistence. WIFI_PS_NONE aborts in coex_enable() when NimBLE starts.
  // Use the driver API: WiFi.setSleep() also returns false when unchanged.
  const esp_err_t powerSaveResult = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  if (powerSaveResult != ESP_OK) {
    Serial.printf("Wi-Fi modem sleep failed: %s; BLE will not start.\n",
                  esp_err_to_name(powerSaveResult));
#ifndef LH_MATTER_BUILD
    while (true) delay(1000);
#endif
  }
  if (powerSaveResult == ESP_OK) Serial.println("Wi-Fi connected; modem sleep enabled for Wi-Fi/BLE coexistence.");
  stateMutex = xSemaphoreCreateMutex();
  if (!stateMutex) { Serial.println("Could not allocate state mutex"); while (true) delay(1000); }
  String saved = prefs.getString("stations", "");
#ifdef LH_MATTER_BUILD
  bool validSavedConfig = !saved.length() && !prefs.isKey("stations");
#endif
  if (saved.length()) {
#ifdef LH_MATTER_BUILD
    DynamicJsonDocument doc(kStationJsonCapacity);
#else
    DynamicJsonDocument doc(16384);
#endif
    std::unique_ptr<StationConfig[]> restored(new StationConfig[kStations]);
    std::string error;
    if (!deserializeJson(doc, saved)) {
      const bool legacy = doc.is<JsonArray>();
      uint32_t counter = legacy ? 6 : doc["next_id"].as<uint32_t>();
      if (parseStationList(legacy ? doc.as<JsonVariantConst>() : doc["stations"].as<JsonVariantConst>(),
                           restored.get(), nullptr, counter, true, legacy, error)) {
        for (unsigned i = 0; i < kStations; ++i) configs[i] = restored[i];
        nextHueId = counter;
#ifdef LH_MATTER_BUILD
        validSavedConfig = true;
#endif
      } else Serial.println("Saved station configuration is invalid; please assign the stations again.");
    }
  }
#ifdef LH_MATTER_BUILD
  saved = String(); // Free the boot configuration text before reserving resources.
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
#else
  Serial.println("Initializing BLE ...");
  if (!NimBLEDevice::init("Lighthouse Bridge")) {
    Serial.println("BLE initialization failed.");
    while (true) delay(1000);
  }
  Serial.println("BLE ready (NimBLE 2.5.2, connect_timeout_ms=3000, establishment_retry_limit=1).");
  if (xTaskCreatePinnedToCore(bleWorker, "lighthouse-ble", 8192, nullptr, 1, nullptr, 0) != pdPASS) {
    Serial.println("Could not start BLE task"); while (true) delay(1000);
  }
  configureRoutes();
  alexaReady = alexa.begin(&server);
  if (!alexaReady) { Serial.println("SSDP startup failed; the web interface remains available."); server.begin(); }
  if (MDNS.begin(hostname.c_str())) MDNS.addService("http", "tcp", 80);
#endif
  Serial.printf("Web interface: http://%s/ or http://%s.local/\n", WiFi.localIP().toString().c_str(), hostname.c_str());
  servicesStarted = true;
#ifdef LH_MATTER_BUILD
  Serial.println(bleWorkerAvailable.load() ? "Lighthouse Bridge ready." : "HTTP recovery ready; Bluetooth unavailable.");
  Serial.printf("Memory ready: free_8bit=%u, largest_8bit=%u, minimum_8bit=%u, status_entry_bytes=%u\n",
    matterDataFree(),matterDataLargest(),heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
    static_cast<unsigned>(sizeof(StatusStation)));
#else
  Serial.println("Lighthouse Bridge ready.");
#endif
  emitDeviceInfo();
}

void setup() {
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  delay(500);
  prefs.begin("lhbridge", false);
  WiFi.setSleep(WIFI_PS_MIN_MODEM);
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
#ifdef LH_MATTER_BUILD
  WiFi.enableIPv6();
#endif
  accessKey = prefs.getString("access_key", "");
  if (accessKey.length() != 24) {
    accessKey = randomHex(12);
    if (prefs.putString("access_key", accessKey) != accessKey.length()) {
      Serial.println("NVS_KEY_SAVE_FAILED"); while (true) delay(1000);
    }
  }
  const String authentication = prefs.getString("web_auth", "");
  if (authentication.length()) {
    StaticJsonDocument<256> doc;
    if (!deserializeJson(doc, authentication)) {
      webSalt = doc["salt"].as<String>(); webHash = doc["hash"].as<String>();
      if (webSalt.length() != 32 || webHash.length() != 64) { webSalt = ""; webHash = ""; }
    }
  }
#ifdef LH_MATTER_BUILD
  // NetworkInterface::macAddress can return zeros before its netif exists.
  // Derive the same STA suffix independently of asynchronous Wi-Fi startup.
  uint8_t stationMac[6]{};
  char suffixBytes[7];
  if (esp_read_mac(stationMac, ESP_MAC_WIFI_STA) != ESP_OK) {
    Serial.println("DEVICE_MAC_READ_FAILED");
    ESP.restart();
    return;
  }
  snprintf(suffixBytes, sizeof(suffixBytes), "%02X%02X%02X", stationMac[3], stationMac[4], stationMac[5]);
  String suffix = suffixBytes;
#else
  String suffix = WiFi.macAddress(); suffix.replace(":", ""); suffix = suffix.substring(6);
#endif
  apName = "Lighthouse-Setup-" + suffix;
  hostname = "lighthouse-" + suffix; hostname.toLowerCase();
  customDeviceName = prefs.getString("dev_name", "");
  if (validDeviceName(customDeviceName.c_str())) hostname = apName = customDeviceName;
  else customDeviceName = "";
  WiFi.setHostname(hostname.c_str());
  Serial.printf("\nLighthouse Bridge %s\n", LH_FIRMWARE_VERSION);
  Serial.println("Setup Wi-Fi / API key: " + accessKey);
  Serial.println("Web user: admin; first login uses the key, then choose your own password.");
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
  });
  wifiManager.setHostname(hostname);
  wifiManager.setCustomHeadElement("<meta name='color-scheme' content='dark'><style>"
    "body{margin:0;background:#101817;color:#edf5ef;font:16px/1.5 system-ui,sans-serif}"
    ".wrap{display:block;box-sizing:border-box;min-width:0;width:calc(100% - 32px);max-width:560px;margin:32px auto;padding:24px;background:#1b2624;border:1px solid #33433f;border-radius:16px}"
    "h1{letter-spacing:-1px}a{color:#c2f78c}a:hover{color:#edf5ef}.q{filter:invert(1)}button,input,select{box-sizing:border-box;font:inherit;border-radius:10px;padding:12px}"
    "button{background:#c2f78c;color:#15210f;border:1px solid #c2f78c;font-weight:600}"
    "input,select{background:#101817;color:#edf5ef;border:1px solid #33433f}.msg{background:#1b2624;border-radius:10px}"
    "input:focus,button:focus{outline:2px solid #c2f78c;outline-offset:2px}</style>");
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConnectTimeout(20);
  wifiManager.setConfigPortalTimeout(0);
  wifiManager.setSaveConfigCallback(persistWifiCredentials);
  wifiManager.autoConnect(apName.c_str(), accessKey.c_str());
  // The main loop services USB info while the setup portal waits for Wi-Fi.
}

void loop() {
  processSerial();
  wifiManager.process();
  if (restartPending && millis() - restartAt >= 1500) ESP.restart();
  if (otaPrepared && !otaUploading && !otaSucceeded && millis() - otaPreparedAt >= 60000) cancelOta("OTA_ABORTED");
  if (manualPortal) {
    if (!wifiManager.getConfigPortalActive()) {
      if (WiFi.status() == WL_CONNECTED) persistWifiCredentials();
      ESP.restart();
    }
    delay(2); return;
  }
  if (!servicesStarted) {
    if (WiFi.status() == WL_CONNECTED) {
      if (wifiManager.getConfigPortalActive()) wifiManager.stopConfigPortal();
      startServices();
    }
    delay(2); return;
  }
#ifdef LH_MATTER_BUILD
  server.handleClient();
  matterBridgeLoop();
  alexaReady = matterReady.load() && bleWorkerAvailable.load();
#else
  alexa.loop();
#endif
  static uint32_t pressedAt = 0;
  if (digitalRead(0) == LOW) {
    if (!pressedAt) pressedAt = millis();
    if (millis() - pressedAt >= 5000) {
      Lock lock;
      if (!otaPrepared && !occupied()) { blePaused = true; portalRequested = true; }
    }
  } else pressedAt = 0;
  if (portalRequested && !bleActive) {
    portalRequested = false;
    server.stop();
#ifndef LH_MATTER_BUILD
    MDNS.end();
#endif
    manualPortal = true;
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.startConfigPortal(apName.c_str(), accessKey.c_str());
  }
  delay(2);
}
