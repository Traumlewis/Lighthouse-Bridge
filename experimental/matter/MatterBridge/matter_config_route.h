#pragma once
static_assert(kConfigRequestCapacity + sizeof(StationConfig)*kStations <= 4096,
              "Bound the Matter save parser/config workspace on the existing HTTP stack");
void matterSaveConfiguration() {
  if (!mutationAllowed()) return;
  if (blePaused || otaPrepared) { fail(409,"BLE_BUSY"); return; }
  StationConfig next[kStations];
  uint32_t counter = nextHueId;
  std::string error;
  int status = 400;
  {
    String body = server.takePlainBody();
    StaticJsonDocument<kConfigRequestCapacity> doc;
    if (body.length() > 8192) { error = "REQUEST_TOO_LARGE"; status = 413; }
    else {
      const auto parsed = deserializeJson(doc,body.begin()); // Mutable, owned, zero-copy input.
      if (parsed == DeserializationError::NoMemory) { error = "OUT_OF_MEMORY"; status = 503; }
      else if (parsed) error = "JSON_INVALID";
      else parseStationList(doc["stations"],next,configs,counter,false,false,error);
    }
  } // Release request body and parser workspace before registry/NVS serialization.
  if (!error.empty()) {
    Serial.printf("Config save: phase=parse, accepted=false, free_8bit=%u, largest_8bit=%u\n",matterDataFree(),matterDataLargest());
    fail(status,error.c_str()); return;
  }
  bool persisted = false, busy = false;
  {
    Lock lock;
    busy = occupied(false) || matterInitializing || restartPending;
    if (!busy && matterPrepareConfiguration(next,error)) {
      String saved;
      if (!makeConfigString(saved,[&](auto& writer) { return streamConfiguration(writer,next,counter); }))
        error = "OUT_OF_MEMORY";
      else persisted = prefs.putString("stations",saved) == saved.length();
      if (persisted) {
        // Keep the live assignment/measurements paired until restart. The reply
        // returns the proposed fixed IDs; it never applies a stale BLE result.
        blePaused = true; matterReady = false; matterError = "RESTARTING";
        restartPending = true; restartAt = millis();
      }
    }
  }
  if (!persisted) {
    Serial.printf("Config save: phase=persist, accepted=false, free_8bit=%u, largest_8bit=%u\n",matterDataFree(),matterDataLargest());
    fail(busy ? 409 : 503,busy ? "BLE_BUSY" : error.empty() ? "SAVE_FAILED" : error.c_str()); return;
  }
  CountSink count; JsonStreamWriter counterWriter(count);
  streamConfiguration(counterWriter,next,counter,true);
  server.sendHeader("Cache-Control","no-store"); server.setContentLength(count.size);
  server.send(200,"application/json; charset=utf-8","");
  JsonHttpSink sink; JsonStreamWriter writer(sink);
  const bool complete = streamConfiguration(writer,next,counter,true);
  if (!complete) server.client().stop();
  restartAt = millis(); // Give the complete reply the full restart grace period.
  Serial.printf("Config save: phase=reply, accepted=true, complete=%s, free_8bit=%u, largest_8bit=%u\n",
                complete ? "true" : "false",matterDataFree(),matterDataLargest());
}
