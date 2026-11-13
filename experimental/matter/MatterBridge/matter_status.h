#pragma once
// Included after production state/auth and the Matter adapter are defined.
void matterStatusRoute() {
  if (!authenticated()) return;
  std::unique_ptr<StatusSnapshot> view(new(std::nothrow) StatusSnapshot);
  if (!view) {
    Serial.printf("Status snapshot allocation failed: bytes=%u, free=%u, largest=%u\n",
      static_cast<unsigned>(sizeof(StatusSnapshot)),ESP.getFreeHeap(),ESP.getMaxAllocHeap());
    fail(503,"OUT_OF_MEMORY"); return;
  }
  {
    Lock lock;
    view->now = millis();
    std::copy(configs,configs+kStations,view->configs);
    std::copy(states,states+kStations,view->states);
    std::copy(refreshRequested,refreshRequested+kStations,view->readQueued);
    std::copy(found,found+kStations,view->found);
    view->foundCount = foundCount; view->scanFinishedMs = scanFinishedMs;
    view->scanning = scanning || scanPending; view->foundTruncated = foundTruncated;
    view->maintenance = blePaused || portalRequested || otaPrepared;
  }
  StaticJsonDocument<JSON_OBJECT_SIZE(24)+256> header;
  header["version"] = LH_FIRMWARE_VERSION; header["alexa_protocol"] = "matter";
  header["matter_ready"] = matterReady.load() && bleWorkerAvailable.load(); header["matter_paired"] = matterPaired.load();
  header["matter_error"] = matterError.load(); header["poll_interval_ms"] = 15000;
  header["max_stations"] = kStations;
  header["ble_worker_available"] = bleWorkerAvailable.load(); header["ble_startup_error"] = bleStartupError.load();
  header["ota_supported"] = esp_ota_get_next_update_partition(nullptr) != nullptr;
  header["ota_product"] = kOtaProduct; header["setup_ssid"] = apName;
  header["maintenance"] = view->maintenance; header["ip"] = WiFi.localIP().toString();
  header["hostname"] = hostname; header["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  header["alexa_started"] = matterReady.load() && bleWorkerAvailable.load(); header["free_heap"] = ESP.getFreeHeap();
  header["largest_free_block"] = ESP.getMaxAllocHeap(); header["min_free_heap"] = ESP.getMinFreeHeap();
  header["uptime_ms"] = view->now; header["scanning"] = view->scanning;
  header["scan_finished_ms"] = view->scanFinishedMs; header["scan_truncated"] = view->foundTruncated;
  if (header.overflowed()) { fail(503,"OUT_OF_MEMORY"); return; }
  server.sendHeader("Cache-Control","no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200,"application/json; charset=utf-8","");
  JsonHttpSink sink; StatusWriter<JsonHttpSink> writer(sink);
  if (streamStatus(header,*view,writer,true)) server.sendContent("");
  else server.client().stop();
}
