#include <Arduino.h>
#include <ArduinoJson.h>
#include <Espalexa.h>
#include <ESPmDNS.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>
#include "bridge_core.h"
#include "web_ui.h"
#include "password_hash.h"

using namespace bridge;
WebServer server(80);
Espalexa alexa;
Preferences prefs;
SemaphoreHandle_t stateMutex;
StationConfig configs[kStations];
StationState states[kStations];
EspalexaDevice* lights[kStations]{};
EspalexaDevice* groupLight = nullptr;
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
struct FoundStation { char name[48]; char mac[18]; uint8_t type; int rssi; };
FoundStation found[16]{};
unsigned foundCount = 0;
bool foundTruncated = false;

struct Lock {
  Lock() { xSemaphoreTake(stateMutex, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(stateMutex); }
};

void sendJson(int code, const JsonDocument& doc) {
  String body;
  serializeJson(doc, body);
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json; charset=utf-8", body);
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
  if (server.arg("plain").length() > 2400) { fail(413, "REQUEST_TOO_LARGE"); return false; }
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
  Lock lock;
  bool accepted = false;
  for (unsigned i = 0; i < kStations; ++i) {
    if ((slot < 0 || slot == static_cast<int>(i)) && configs[i].mac[0]) {
      request(states[i], target);
      accepted = true;
    }
  }
  return accepted;
}

// BLE is confined to this worker: scan, connect, write and disconnect never
// block the HTTP/SSDP loop. One connection at a time for all four stations.
void refreshRetryAddress(StationConfig& config, unsigned slot) {
  // A failed establishment (HCI 0x3e) needs a new radio opportunity, not a
  // longer GATT read timeout. Passive scanning avoids extra scan requests.
  auto* scan = NimBLEDevice::getScan();
  scan->setActiveScan(false);
  scan->setInterval(97);
  scan->setWindow(67);
  auto results = scan->start(2, false);
  bool seen = false;
  for (int i = 0; i < results.getCount(); ++i) {
    auto device = results.getDevice(i);
    char mac[18]{};
    normalizeMac(device.getAddress().toString().c_str(), mac);
    if (strcmp(mac, config.mac) || !device.isConnectable()) continue;
    // Use the actual advertised address type for this job, without changing
    // saved assignments or accepting a different device based on its name.
    config.addressType = device.getAddressType();
    Serial.printf("Station %u: retry scan: advertisement seen, RSSI=%d dBm, address_type=%u\n",
                  slot + 1, device.getRSSI(), config.addressType);
    seen = true;
    break;
  }
  if (!seen) Serial.printf("Station %u: retry scan: no connectable advertisement within 2 seconds\n", slot + 1);
  scan->clearResults();
}

void logBleFailure(unsigned slot, const char* error, int code, unsigned attempt) {
  Serial.printf("Station %u: %s, BLE code=%d, attempt=%u%s\n", slot + 1,
                error, code, attempt + 1,
                code == BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT)
                  ? " (HCI 0x3e: connection establishment failed)" : "");
}

bool accessPower(const StationConfig& config, const Target* target, int& observed, char* error, int& bleError) {
  bleError = 0;
  NimBLEClient* client = NimBLEDevice::createClient();
  if (!client) { strcpy(error, "BLE_NO_CLIENT"); return false; }
  client->setConnectTimeout(target ? 3 : 6);
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
        const std::string value = power->readValue();
        if (!value.empty()) observed = static_cast<uint8_t>(value[0]);
      }
      if (ok && observed < 0) {
        strcpy(error, "BLE_READ_FAILED");
        if (!target) ok = false;
      }
    }
  }
  if (!ok || observed < 0) bleError = client->getLastError();
  if (client->isConnected()) client->disconnect();
  NimBLEDevice::deleteClient(client);
  return ok;
}

void discoverStations() {
  auto* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(97);
  scan->setWindow(67);
  auto results = scan->start(5, false);
  FoundStation next[16]{};
  unsigned count = 0;
  bool truncated = false;
  for (int i = 0; i < results.getCount(); ++i) {
    auto device = results.getDevice(i);
    const std::string name = device.getName();
    if (name.compare(0, 4, "LHB-") != 0) continue;
    if (count == 16) { truncated = true; continue; }
    auto& item = next[count++];
    strncpy(item.name, name.c_str(), sizeof(item.name) - 1);
    normalizeMac(device.getAddress().toString().c_str(), item.mac);
    item.type = device.getAddressType();
    item.rssi = device.getRSSI();
  }
  scan->clearResults();
  {
    Lock lock;
    memcpy(found, next, sizeof(found));
    foundCount = count;
    foundTruncated = truncated;
    scanning = false;
    scanFinishedMs = millis();
  }
}

void bleWorker(void*) {
  unsigned cursor = 0;
  for (;;) {
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
          const uint32_t interval = readInterval(state);
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
              refreshRetryAddress(config, selected);
            }
            { Lock lock; for (auto& state : states) urgent |= state.pending;
              urgent |= blePaused || states[selected].generation != generation; }
            if (urgent) break;
          }
          error[0] = '\0';
          if (accessPower(config, nullptr, observed, error, bleError)) {
            if (attempt) Serial.printf("Station %d: read recovered on attempt=%u, observed=%d\n",
                                       selected + 1, attempt + 1, observed);
            break;
          }
          logBleFailure(selected, error, bleError, attempt);
        }
        {
          Lock lock; finishRead(states[selected], generation, observed, millis(), error, bleError);
        }
      } else {
      for (unsigned attempt = 0; attempt < 3; ++attempt) {
        { Lock lock; if (states[selected].generation != generation) break; }
        if (attempt && bleError == BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT)) {
          refreshRetryAddress(config, selected);
          { Lock lock; if (states[selected].generation != generation) break; }
        }
        error[0] = '\0';
        sent = accessPower(config, &target, observed, error, bleError);
        if (sent) break;
        logBleFailure(selected, error, bleError, attempt);
        if (attempt < 2) vTaskDelay(pdMS_TO_TICKS(600 * (attempt + 1) + esp_random() % 301));
      }
      {
        Lock lock;
        finish(states[selected], generation, sent, observed, millis(), error);
        states[selected].bleError = bleError;
      }
      Serial.printf("Station %d: %s, %s\n", selected + 1, targetName(target), sent ? "sent" : error);
      }
      cursor = (selected + 1) % kStations;
    }
    { Lock lock; bleActive = false; }
    vTaskDelay(pdMS_TO_TICKS(selected >= 0 ? 150 : 20));
  }
}

bool parseConfigs(JsonVariantConst input, StationConfig* out, String& error) {
  if (!input.is<JsonArrayConst>() || input.size() != kStations) {
    error = "CONFIG_FOUR"; return false;
  }
  for (unsigned i = 0; i < kStations; ++i) {
    const auto item = input[i];
    if (!item["name"].is<const char*>() || !validName(item["name"].as<const char*>())) {
      error = "CONFIG_NAME"; return false;
    }
    if (!item["mac"].is<const char*>() || !item["address_type"].is<int>()) {
      error = "CONFIG_MAC"; return false;
    }
    const int type = item["address_type"].as<int>();
    if (type != 0 && type != 1) { error = "CONFIG_TYPE"; return false; }
    strcpy(out[i].name, item["name"].as<const char*>());
    const char* mac = item["mac"];
    if (*mac && !normalizeMac(mac, out[i].mac)) { error = "CONFIG_MAC"; return false; }
    out[i].addressType = type;
    for (unsigned j = 0; j < i; ++j) {
      if (*mac && strcmp(out[i].mac, out[j].mac) == 0) { error = "CONFIG_MAC"; return false; }
      if (strcmp(out[i].name, out[j].name) == 0) { error = "CONFIG_NAME"; return false; }
    }
    if (strcmp(out[i].name, "VR Basisstationen") == 0) { error = "CONFIG_GROUP_NAME"; return false; }
  }
  return true;
}

void statusRoute() {
  if (!authenticated()) return;
  DynamicJsonDocument doc(8192);
  doc["version"] = "0.2.2";
  doc["poll_interval_ms"] = 15000;
  doc["ip"] = WiFi.localIP().toString();
  doc["hostname"] = hostname;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["alexa_started"] = alexaReady;
  doc["uptime_ms"] = millis();
  doc["free_heap"] = ESP.getFreeHeap();
  {
    Lock lock;
    doc["scanning"] = scanning || scanPending;
    doc["scan_finished_ms"] = scanFinishedMs;
    doc["scan_truncated"] = foundTruncated;
    auto stations = doc.createNestedArray("stations");
    for (unsigned i = 0; i < kStations; ++i) {
      auto item = stations.createNestedObject();
      item["id"] = i + 1;
      item["name"] = configs[i].name;
      item["mac"] = configs[i].mac;
      item["address_type"] = configs[i].addressType;
      item["configured"] = configs[i].mac[0] != '\0';
      const auto& state = states[i];
      if (state.hasRequest) item["requested"] = targetName(state.requested);
      else item["requested"] = nullptr;
      item["pending"] = state.pending;
      item["busy"] = state.busy;
      item["reading"] = state.reading || refreshRequested[i];
      item["reachable"] = freshMeasurement(state, millis());
      item["read_failures"] = state.readFailures;
      item["ble_error"] = state.bleError;
      item["last_read_attempt_ms"] = state.lastReadAttemptMs;
      item["command_sent"] = state.sent;
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
}

// Hue has no local push-status channel. Answer Alexa's GETs from the exact
// same measured BLE state used by /api/status, including reachability.
bool serveHueMeasurements(const String& uri) {
  if (server.method() != HTTP_GET) return false;
  const int position = uri.indexOf("/lights", 5);
  if (position < 0) return false;
  String tail = uri.substring(position + 7);
  const bool stateOnly = tail.endsWith("/state");
  if (stateOnly) tail.remove(tail.length() - 6);
  if (tail.startsWith("/")) tail.remove(0, 1);
  String mac = WiFi.macAddress(), suffix = mac; suffix.replace(":", "");
  const uint32_t mac24 = strtoul(suffix.substring(6).c_str(), nullptr, 16);
  int selected = -1;
  if (tail.length()) {
    char* end = nullptr;
    const long key = strtol(tail.c_str(), &end, 10);
    if (!end || *end || (static_cast<uint32_t>(key) >> 7) != mac24 ||
        (static_cast<uint32_t>(key) & 127) > kStations) {
      StaticJsonDocument<32> empty; empty.to<JsonObject>(); sendJson(200, empty); return true;
    }
    selected = static_cast<uint32_t>(key) & 127;
  }
  DynamicJsonDocument doc(4096);
  JsonObject root = doc.to<JsonObject>();
  {
    Lock lock;
    bool any = false, allOn = true, allReachable = true;
    for (unsigned i = 0; i < kStations; ++i) {
      if (!configs[i].mac[0]) continue;
      any = true; allOn &= measuredOn(states[i]);
      allReachable &= freshMeasurement(states[i], millis());
      if (!states[i].reading && !states[i].busy &&
          millis() - states[i].lastReadAttemptMs >= readInterval(states[i])) refreshRequested[i] = true;
    }
    for (unsigned i = 0; i <= kStations; ++i) {
      if (selected >= 0 && selected != static_cast<int>(i)) continue;
      const int32_t key = static_cast<int32_t>((mac24 << 7) | i);
      JsonObject light = selected < 0 ? root.createNestedObject(String(key)) : root;
      JsonObject state = stateOnly ? light : light.createNestedObject("state");
      state["on"] = i < kStations ? measuredOn(states[i]) : any && allOn;
      state["reachable"] = i < kStations ? configs[i].mac[0] && freshMeasurement(states[i], millis()) : any && allReachable;
      state["alert"] = "none";
      if (!stateOnly) {
        char unique[32]; snprintf(unique, sizeof(unique), "%s-%02X-00:11", mac.c_str(), i + 1);
        light["type"] = "On/off light";
        light["name"] = i < kStations ? configs[i].name : "VR Basisstationen";
        light["modelid"] = "HASS321"; light["manufacturername"] = "Philips";
        light["uniqueid"] = unique; light["swversion"] = "lighthouse-bridge-0.2.0";
      }
    }
  }
  sendJson(200, doc); return true;
}

void configureRoutes() {
  const char* headers[] = {"X-API-Key", "Origin", "Content-Type", "Cookie"};
  server.collectHeaders(headers, 4);
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.sendHeader("X-Frame-Options", "DENY");
    server.send_P(200, "text/html; charset=utf-8", WEB_UI);
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
    StaticJsonDocument<384> doc;
    doc["password_required"] = !webHash.length();
    doc["setup_ssid"] = apName;
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
    { Lock lock; for (unsigned i = 0; i < kStations; ++i) if (configs[i].mac[0]) refreshRequested[i] = true; }
    StaticJsonDocument<64> doc; doc["accepted"] = true; sendJson(202, doc);
  });
  server.on("/api/status", HTTP_GET, statusRoute);
  server.on("/api/power", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    StaticJsonDocument<256> doc;
    if (!readBody(doc)) return;
    Target target;
    if (!doc["state"].is<const char*>() || !parseTarget(doc["state"], target)) {
      fail(400, "STATE_INVALID"); return;
    }
    int slot = -1;
    if (doc["station"].is<int>() && doc["station"].as<int>() >= 1 && doc["station"].as<int>() <= 4)
      slot = doc["station"].as<int>() - 1;
    else if (!doc["station"].is<const char*>() || strcmp(doc["station"], "all") != 0) {
      fail(400, "STATION_INVALID"); return;
    }
    if (!queueTarget(slot, target)) { fail(409, "NO_STATION"); return; }
    doc.clear(); doc["accepted"] = true;
    sendJson(202, doc);
  });
  server.on("/api/scan", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    { Lock lock; if (!scanning) scanPending = true; }
    StaticJsonDocument<64> doc; doc["accepted"] = true; sendJson(202, doc);
  });
  server.on("/api/config", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    DynamicJsonDocument doc(3072);
    if (!readBody(doc)) return;
    StationConfig next[kStations]{};
    String error;
    if (!parseConfigs(doc["stations"], next, error)) { fail(400, error.c_str()); return; }
    String saved;
    serializeJson(doc["stations"], saved);
    bool busy = false, persisted = false;
    {
      Lock lock;
      busy = occupied(false);
      if (!busy) {
        persisted = prefs.putString("stations", saved) == saved.length();
        if (persisted) {
          for (unsigned i = 0; i < kStations; ++i) {
            if (strcmp(configs[i].mac, next[i].mac) || configs[i].addressType != next[i].addressType) {
              const uint32_t nextGeneration = states[i].generation + 1;
              states[i] = StationState{};
              states[i].generation = nextGeneration;
            }
            configs[i] = next[i];
          }
        }
      }
    }
    if (busy) { fail(409, "BLE_BUSY"); return; }
    if (!persisted) { fail(500, "SAVE_FAILED"); return; }
    for (unsigned i = 0; i < kStations; ++i) lights[i]->setName(next[i].name);
    doc.clear(); doc["saved"] = true; sendJson(200, doc);
  });
  server.on("/api/wifi", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    { Lock lock; if (occupied()) { fail(409, "BLE_BUSY"); return; } blePaused = true; portalRequested = true; }
    StaticJsonDocument<96> doc; doc["setup_ssid"] = apName; doc["accepted"] = true;
    sendJson(202, doc);
  });
  // Espalexa needs its Hue-compatible endpoints without our admin login.
  server.onNotFound([] {
    const String uri = server.uri();
    if (uri == "/api/status" || uri == "/api/power" || uri == "/api/config" ||
        uri == "/api/scan" || uri == "/api/wifi" || uri == "/api/refresh" ||
        uri == "/api/login" || uri == "/api/logout" || uri == "/api/password" || uri == "/api/account") {
      if (authenticated()) fail(404, "NOT_FOUND");
      return;
    }
    // Hue also uses /api/<username>/lights. Do not intercept it as our API.
    if (uri == "/api" || uri.startsWith("/api/")) {
      if (server.header("Origin").length()) { fail(403, "ORIGIN_DENIED"); return; }
      if (serveHueMeasurements(uri)) return;
      if (alexa.handleAlexaApiCall(uri, server.arg("plain"))) return;
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
  doc["protocol"] = 1; doc["version"] = "0.2.2";
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
    while (true) delay(1000);
  }
  Serial.println("Wi-Fi connected; modem sleep enabled for Wi-Fi/BLE coexistence.");
  stateMutex = xSemaphoreCreateMutex();
  if (!stateMutex) { Serial.println("Could not allocate state mutex"); while (true) delay(1000); }
  for (unsigned i = 0; i < kStations; ++i)
    snprintf(configs[i].name, sizeof(configs[i].name), "Base station %u", i + 1);
  const String saved = prefs.getString("stations", "");
  if (saved.length()) {
    DynamicJsonDocument doc(3072); StationConfig restored[kStations]{}; String error;
    if (!deserializeJson(doc, saved) && parseConfigs(doc.as<JsonVariantConst>(), restored, error))
      memcpy(configs, restored, sizeof(configs));
    else Serial.println("Saved station configuration is invalid; please assign the stations again.");
  }
  Serial.println("Initializing BLE ...");
  NimBLEDevice::init("Lighthouse Bridge");
  Serial.println("BLE ready.");
  if (xTaskCreatePinnedToCore(bleWorker, "lighthouse-ble", 8192, nullptr, 1, nullptr, 0) != pdPASS) {
    Serial.println("Could not start BLE task"); while (true) delay(1000);
  }
  for (unsigned i = 0; i < kStations; ++i) {
    lights[i] = new EspalexaDevice(configs[i].name, [i](EspalexaDevice* device) {
      queueTarget(i, device->getState() ? Target::On : Target::Sleep);
    }, EspalexaDeviceType::onoff);
    alexa.addDevice(lights[i]);
  }
  groupLight = new EspalexaDevice("VR Basisstationen", [](EspalexaDevice* device) {
    queueTarget(-1, device->getState() ? Target::On : Target::Sleep);
  }, EspalexaDeviceType::onoff);
  alexa.addDevice(groupLight);
  configureRoutes();
  alexaReady = alexa.begin(&server);
  if (!alexaReady) { Serial.println("SSDP startup failed; the web interface remains available."); server.begin(); }
  if (MDNS.begin(hostname.c_str())) MDNS.addService("http", "tcp", 80);
  Serial.printf("Web interface: http://%s/ or http://%s.local/\n", WiFi.localIP().toString().c_str(), hostname.c_str());
  servicesStarted = true;
  Serial.println("Lighthouse Bridge ready.");
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
  String suffix = WiFi.macAddress(); suffix.replace(":", ""); suffix = suffix.substring(6);
  apName = "Lighthouse-Setup-" + suffix;
  hostname = "lighthouse-" + suffix; hostname.toLowerCase();
  WiFi.setHostname(hostname.c_str());
  Serial.println("\nLighthouse Bridge 0.2.2");
  Serial.println("Setup Wi-Fi / API key: " + accessKey);
  Serial.println("Web user: admin; first login uses the key, then choose your own password.");
  emitDeviceInfo();
  wifiManager.setDebugOutput(false);
  wifiManager.setTitle("Lighthouse Wi-Fi Setup");
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
  alexa.loop();
  // Alexa and the dashboard share measured state; requested targets are separate.
  bool allOn = true, anyConfigured = false;
  {
    Lock lock;
    for (unsigned i = 0; i < kStations; ++i) {
      const auto& state = states[i];
      const bool on = measuredOn(state);
      lights[i]->setState(on);
      if (configs[i].mac[0]) { anyConfigured = true; allOn &= on; }
    }
  }
  groupLight->setState(anyConfigured && allOn);
  static uint32_t pressedAt = 0;
  if (digitalRead(0) == LOW) {
    if (!pressedAt) pressedAt = millis();
    if (millis() - pressedAt >= 5000) {
      Lock lock;
      if (!occupied()) { blePaused = true; portalRequested = true; }
    }
  } else pressedAt = 0;
  if (portalRequested) {
    portalRequested = false;
    server.stop(); MDNS.end();
    manualPortal = true;
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.startConfigPortal(apName.c_str(), accessKey.c_str());
  }
  delay(2);
}
