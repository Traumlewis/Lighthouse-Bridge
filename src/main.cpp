#include <Arduino.h>
#include <ArduinoJson.h>
#include <Espalexa.h>
#include <ESPmDNS.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "bridge_core.h"
#include "web_ui.h"

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
bool portalRequested = false;
bool alexaReady = false;
bool scanPending = false, scanning = false;
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
bool authenticated() {
  if (server.header("X-API-Key") == accessKey || server.authenticate("admin", accessKey.c_str()))
    return true;
  server.requestAuthentication(BASIC_AUTH, "Lighthouse Bridge");
  return false;
}
bool mutationAllowed() {
  if (!authenticated()) return false;
  const String origin = server.header("Origin");
  if (origin.length() && origin != "http://" + server.hostHeader()) {
    fail(403, "Fremder Ursprung nicht erlaubt"); return false;
  }
  if (!server.header("Content-Type").startsWith("application/json")) {
    fail(415, "Content-Type application/json erforderlich"); return false;
  }
  return true;
}
bool readBody(JsonDocument& doc) {
  if (server.arg("plain").length() > 2400) { fail(413, "Anfrage zu gross"); return false; }
  if (deserializeJson(doc, server.arg("plain"))) { fail(400, "Ungueltiges JSON"); return false; }
  return true;
}
bool occupied() { // Caller holds stateMutex.
  if (scanning || scanPending) return true;
  for (auto& state : states) if (state.busy || state.pending) return true;
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
bool sendPower(const StationConfig& config, Target target, int& observed, char* error) {
  NimBLEClient* client = NimBLEDevice::createClient();
  if (!client) { strcpy(error, "Kein BLE-Client verfuegbar"); return false; }
  client->setConnectTimeout(5);
  bool ok = false;
  if (!client->connect(NimBLEAddress(std::string(config.mac), config.addressType))) {
    strcpy(error, "BLE-Verbindung fehlgeschlagen");
  } else {
    auto* service = client->getService(kService);
    auto* power = service ? service->getCharacteristic(kPower) : nullptr;
    if (!power || (!power->canWriteNoResponse() && !power->canWrite())) {
      strcpy(error, "Lighthouse-V2-Power-Characteristic fehlt");
    } else {
      const uint8_t command = static_cast<uint8_t>(target); // Only 0x00 / 0x01.
      ok = power->writeValue(&command, 1, !power->canWriteNoResponse());
      if (!ok) strcpy(error, "BLE-Schreibvorgang fehlgeschlagen");
      if (ok && power->canRead()) {
        vTaskDelay(pdMS_TO_TICKS(250));
        const std::string value = power->readValue();
        if (!value.empty()) observed = static_cast<uint8_t>(value[0]);
      }
    }
  }
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
    {
      Lock lock;
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
    }
    if (doScan) discoverStations();
    if (selected >= 0) {
      char error[96]{};
      int observed = -1;
      bool sent = false;
      for (unsigned attempt = 0; attempt < 3; ++attempt) {
        { Lock lock; if (states[selected].generation != generation) break; }
        error[0] = '\0';
        sent = sendPower(config, target, observed, error);
        if (sent) break;
        if (attempt < 2) vTaskDelay(pdMS_TO_TICKS(600 * (attempt + 1)));
      }
      {
        Lock lock;
        finish(states[selected], generation, sent, observed, millis(), error);
      }
      Serial.printf("Station %d: %s, %s\n", selected + 1, targetName(target), sent ? "gesendet" : error);
      cursor = (selected + 1) % kStations;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

bool parseConfigs(JsonVariantConst input, StationConfig* out, String& error) {
  if (!input.is<JsonArrayConst>() || input.size() != kStations) {
    error = "Genau vier Stationen erforderlich"; return false;
  }
  for (unsigned i = 0; i < kStations; ++i) {
    const auto item = input[i];
    if (!item["name"].is<const char*>() || !validName(item["name"].as<const char*>())) {
      error = "Name: 1-40 Bytes, keine Anfuehrungs-, Steuer- oder XML-Zeichen"; return false;
    }
    if (!item["mac"].is<const char*>() || !item["address_type"].is<int>()) {
      error = "MAC und address_type erforderlich"; return false;
    }
    const int type = item["address_type"].as<int>();
    if (type != 0 && type != 1) { error = "address_type muss 0 oder 1 sein"; return false; }
    strcpy(out[i].name, item["name"].as<const char*>());
    const char* mac = item["mac"];
    if (*mac && !normalizeMac(mac, out[i].mac)) { error = "Ungueltige Bluetooth-MAC"; return false; }
    out[i].addressType = type;
    for (unsigned j = 0; j < i; ++j) {
      if (*mac && strcmp(out[i].mac, out[j].mac) == 0) { error = "MAC doppelt vergeben"; return false; }
      if (strcmp(out[i].name, out[j].name) == 0) { error = "Namen muessen eindeutig sein"; return false; }
    }
    if (strcmp(out[i].name, "VR Basisstationen") == 0) { error = "Name fuer Gesamtschalter reserviert"; return false; }
  }
  return true;
}

void statusRoute() {
  if (!authenticated()) return;
  DynamicJsonDocument doc(8192);
  doc["version"] = "0.1.0";
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

void configureRoutes() {
  const char* headers[] = {"X-API-Key", "Origin", "Content-Type"};
  server.collectHeaders(headers, 3);
  server.on("/", HTTP_GET, [] {
    if (!authenticated()) return;
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("X-Content-Type-Options", "nosniff");
    server.sendHeader("X-Frame-Options", "DENY");
    server.send_P(200, "text/html; charset=utf-8", WEB_UI);
  });
  server.on("/api/status", HTTP_GET, statusRoute);
  server.on("/api/power", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    StaticJsonDocument<256> doc;
    if (!readBody(doc)) return;
    Target target;
    if (!doc["state"].is<const char*>() || !parseTarget(doc["state"], target)) {
      fail(400, "state muss on oder sleep sein"); return;
    }
    int slot = -1;
    if (doc["station"].is<int>() && doc["station"].as<int>() >= 1 && doc["station"].as<int>() <= 4)
      slot = doc["station"].as<int>() - 1;
    else if (!doc["station"].is<const char*>() || strcmp(doc["station"], "all") != 0) {
      fail(400, "station muss 1, 2, 3, 4 oder all sein"); return;
    }
    if (!queueTarget(slot, target)) { fail(409, "Keine konfigurierte Station ausgewaehlt"); return; }
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
      busy = occupied();
      if (!busy) {
        persisted = prefs.putString("stations", saved) == saved.length();
        if (persisted) {
          for (unsigned i = 0; i < kStations; ++i) {
            if (strcmp(configs[i].mac, next[i].mac) || configs[i].addressType != next[i].addressType)
              states[i] = StationState{};
            configs[i] = next[i];
          }
        }
      }
    }
    if (busy) { fail(409, "BLE beschaeftigt; nach Abschluss erneut speichern"); return; }
    if (!persisted) { fail(500, "Konfiguration konnte nicht gespeichert werden"); return; }
    for (unsigned i = 0; i < kStations; ++i) lights[i]->setName(next[i].name);
    doc.clear(); doc["saved"] = true; sendJson(200, doc);
  });
  server.on("/api/wifi", HTTP_POST, [] {
    if (!mutationAllowed()) return;
    { Lock lock; if (occupied()) { fail(409, "BLE beschaeftigt; bitte warten"); return; } }
    portalRequested = true;
    StaticJsonDocument<96> doc; doc["setup_ssid"] = apName; doc["accepted"] = true;
    sendJson(202, doc);
  });
  // Espalexa needs its Hue-compatible endpoints without our admin login.
  server.onNotFound([] {
    const String uri = server.uri();
    if (uri == "/api/status" || uri == "/api/power" || uri == "/api/config" ||
        uri == "/api/scan" || uri == "/api/wifi") {
      if (authenticated()) fail(404, "Unbekannter API-Pfad oder HTTP-Methode");
      return;
    }
    // Hue also uses /api/<username>/lights. Do not intercept it as our API.
    if (uri == "/api" || uri.startsWith("/api/")) {
      if (server.header("Origin").length()) { fail(403, "Hue-API nicht fuer Browseraufrufe"); return; }
      if (alexa.handleAlexaApiCall(uri, server.arg("plain"))) return;
    }
    fail(404, "Nicht gefunden");
  });
}

void setup() {
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  delay(500);
  prefs.begin("lhbridge", false);
  accessKey = prefs.getString("access_key", "");
  if (accessKey.length() != 24) {
    char key[25];
    snprintf(key, sizeof(key), "%08lx%08lx%08lx", (unsigned long)esp_random(),
             (unsigned long)esp_random(), (unsigned long)esp_random());
    accessKey = key;
    if (prefs.putString("access_key", accessKey) != accessKey.length()) {
      Serial.println("NVS-Fehler: Zugangsschluessel nicht gespeichert.");
      while (true) delay(1000);
    }
  }
  WiFi.mode(WIFI_STA);
  String suffix = WiFi.macAddress(); suffix.replace(":", ""); suffix = suffix.substring(6);
  apName = "Lighthouse-Setup-" + suffix;
  hostname = "lighthouse-" + suffix; hostname.toLowerCase();
  Serial.printf("\nLighthouse Bridge 0.1.0\nSetup-WLAN: %s\n", apName.c_str());
  Serial.printf("Setup-Passwort / Web-Passwort / API-Key: %s\nWeb-Benutzer: admin\n", accessKey.c_str());
  Serial.println("Nur ON/SLEEP; keine automatischen Schaltbefehle beim Start.");
  WiFi.setHostname(hostname.c_str());
  WiFiManager wifi;
  wifi.setDebugOutput(false);
  wifi.setTitle("Lighthouse WLAN-Setup");
  wifi.setConnectTimeout(20);
  wifi.setConfigPortalTimeout(0);
  if (!wifi.autoConnect(apName.c_str(), accessKey.c_str())) { ESP.restart(); return; }
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  Serial.printf("Webinterface: http://%s/ oder http://%s.local/\n", WiFi.localIP().toString().c_str(), hostname.c_str());
  stateMutex = xSemaphoreCreateMutex();
  if (!stateMutex) { Serial.println("Kein RAM fuer Mutex"); while (true) delay(1000); }
  for (unsigned i = 0; i < kStations; ++i)
    snprintf(configs[i].name, sizeof(configs[i].name), "Basisstation %u", i + 1);
  const String saved = prefs.getString("stations", "");
  if (saved.length()) {
    DynamicJsonDocument doc(3072); StationConfig restored[kStations]{}; String error;
    if (!deserializeJson(doc, saved) && parseConfigs(doc.as<JsonVariantConst>(), restored, error))
      memcpy(configs, restored, sizeof(configs));
    else Serial.println("Gespeicherte Stationskonfiguration ungueltig; bitte neu zuordnen.");
  }
  NimBLEDevice::init("Lighthouse Bridge");
  if (xTaskCreatePinnedToCore(bleWorker, "lighthouse-ble", 8192, nullptr, 1, nullptr, 0) != pdPASS) {
    Serial.println("BLE-Task konnte nicht gestartet werden"); while (true) delay(1000);
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
  if (!alexaReady) { Serial.println("SSDP-Start fehlgeschlagen; Webinterface bleibt verfuegbar."); server.begin(); }
  if (MDNS.begin(hostname.c_str())) MDNS.addService("http", "tcp", 80);
}

void loop() {
  alexa.loop();
  // Alexa's boolean is an optimistic control value, not a confirmed BLE state.
  bool allOn = true, anyConfigured = false;
  {
    Lock lock;
    for (unsigned i = 0; i < kStations; ++i) {
      const auto& state = states[i];
      const bool on = state.hasRequest ? state.requested == Target::On : state.observed == 0x0b;
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
      if (!occupied()) portalRequested = true;
    }
  } else pressedAt = 0;
  if (portalRequested) {
    // Portal owns port 80 until new credentials have been saved.
    delay(250);
    server.stop();
    MDNS.end();
    WiFiManager wifi;
    wifi.setDebugOutput(false);
    wifi.setTitle("Lighthouse WLAN-Setup");
    wifi.setConfigPortalTimeout(180);
    wifi.setConnectTimeout(20);
    wifi.startConfigPortal(apName.c_str(), accessKey.c_str());
    ESP.restart();
  }
  delay(2);
}
