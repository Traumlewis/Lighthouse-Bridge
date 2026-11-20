#pragma once

bool armRemoteWake() {
  const int first = gpioButtons[0].pin, second = gpioButtons[1].pin;
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  return esp_deep_sleep_enable_gpio_wakeup((1ULL << first) | (1ULL << second),
                                          ESP_GPIO_WAKEUP_GPIO_LOW) == ESP_OK;
#else
  const auto a = static_cast<gpio_num_t>(first), b = static_cast<gpio_num_t>(second);
  for (const auto pin : {a, b}) {
    if (rtc_gpio_pullup_en(pin) != ESP_OK || rtc_gpio_pulldown_dis(pin) != ESP_OK) return false;
  }
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  return esp_sleep_enable_ext1_wakeup_io((1ULL << first) | (1ULL << second),
                                        ESP_EXT1_WAKEUP_ANY_LOW) == ESP_OK;
#else
  // On classic ESP32 EXT1 cannot wake on any low across multiple pins.
  // EXT0 handles the first pin, and EXT1 tests only the second pin.
  return esp_sleep_enable_ext0_wakeup(a, 0) == ESP_OK &&
         esp_sleep_enable_ext1_wakeup_io(1ULL << second, ESP_EXT1_WAKEUP_ALL_LOW) == ESP_OK;
#endif
#endif
}

unsigned remotePressed() {
  return (digitalRead(gpioButtons[0].pin) == LOW ? 1 : 0) |
         (digitalRead(gpioButtons[1].pin) == LOW ? 2 : 0);
}

void runRemoteMode() {
  if (!validRemoteButtons(gpioButtons)) {
    prefs.putBool("remote_mode", false);
    return;
  }
  uint64_t pins = 0;
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  pins = esp_sleep_get_gpio_wakeup_status();
#else
  pins = esp_sleep_get_ext1_wakeup_status();
#if defined(CONFIG_IDF_TARGET_ESP32)
  if (esp_sleep_get_wakeup_causes() & (1U << ESP_SLEEP_WAKEUP_EXT0)) {
    pins |= 1ULL << gpioButtons[0].pin;
  }
#endif
  for (const auto& button : gpioButtons) {
    rtc_gpio_deinit(static_cast<gpio_num_t>(button.pin));
  }
#endif
  applyButtonPins();
  const unsigned wakeMask = ((pins >> gpioButtons[0].pin) & 1) |
                            (((pins >> gpioButtons[1].pin) & 1) << 1);
  RemotePress press(wakeMask, millis());
  RemoteGesture gesture;
  do {
    gesture = press.poll(remotePressed(), millis());
    if (gesture == RemoteGesture::Wait) delay(5);
  } while (gesture == RemoteGesture::Wait);
  bool leave = gesture == RemoteGesture::Exit;
  RemotePress exitPress(0, millis());
  if (gesture == RemoteGesture::First || gesture == RemoteGesture::Second) {
    const auto action = gpioButtons[gesture == RemoteGesture::First ? 0 : 1].action;
    remoteOnly = true;
    const bool bleStarted = loadStations() && NimBLEDevice::init("Lighthouse Bridge");
    if (bleStarted &&
        xTaskCreatePinnedToCore(bleWorker, "lighthouse-ble", 8192, nullptr, 1, nullptr, 0) == pdPASS) {
      bleWorkerAvailable = true;
      const bool turnOn = action == ButtonAction::AllOn;
      queueTarget(-1, turnOn ? Target::On : Target::Sleep, !turnOn);
      for (;;) {
        leave |= exitPress.poll(remotePressed(), millis()) == RemoteGesture::Exit;
        bool done;
        {
          Lock lock;
          done = !occupied();
          for (const auto& state : states) done &= !awaitingConfirmation(state, millis());
          if (done) blePaused = true;
        }
        if (done) break;
        delay(20);
      }
      while (bleWorkerAvailable.load()) delay(10);
    } else {
      Serial.println("REMOTE_BLE_INIT_FAILED");
    }
    if (bleStarted && !NimBLEDevice::deinit(false)) Serial.println("REMOTE_BLE_STOP_FAILED");
  }
  if (!leave && armRemoteWake()) {
    // Wait for stable release so a held input cannot cause a wake loop.
    do {
      gesture = exitPress.poll(remotePressed(), millis());
      if (gesture == RemoteGesture::Wait) delay(5);
    } while (gesture == RemoteGesture::Wait);
    if (gesture != RemoteGesture::Exit) {
      Serial.flush();
      esp_deep_sleep_start();
    }
  }
  if (prefs.putBool("remote_mode", false) != sizeof(uint8_t)) {
    Serial.println("REMOTE_MODE_SAVE_FAILED");
    delay(1000);
  }
  // A fresh boot restores the saved home or local Wi-Fi mode and releases BLE.
  ESP.restart();
}

void configureRemoteRoutes() {
  server.on("/api/remote", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    StaticJsonDocument<96> doc;
    if (!readBody(doc)) return;
    if (doc.size() != 1 || !doc["confirm"].is<bool>() || !doc["confirm"].as<bool>()) {
      fail(400, "REMOTE_CONFIRM_REQUIRED"); return;
    }
    if (!validRemoteButtons(gpioButtons)) { fail(400, "REMOTE_PINS_INVALID"); return; }
    if (!bleWorkerAvailable.load()) { fail(409, "BLE_BUSY"); return; }
    {
      Lock lock;
      if (occupied(false) || blePaused || otaPrepared || restartPending || portalRequested) {
        fail(409, "BLE_BUSY"); return;
      }
      bool assigned = false;
      for (const auto& config : configs) assigned |= config.mac[0] != 0;
      if (!assigned) { fail(409, "NO_STATION"); return; }
      if (prefs.putBool("remote_mode", true) != sizeof(uint8_t)) {
        fail(500, "SAVE_FAILED"); return;
      }
      blePaused = maintenanceWaitIdle = restartPending = true;
      restartAt = millis();
    }
    doc.clear();
    doc["accepted"] = true;
    sendJson(202, doc);
    restartAt = millis();
  });

  server.on("/api/identify", HTTP_POST, [] {
    if (!authenticated(true) || !sameOriginJson()) return;
    StaticJsonDocument<192> doc;
    if (!readBody(doc)) return;
    if (doc.size() != 2 || !doc["id"].is<unsigned>() || !doc["mac"].is<const char*>() ||
        doc["mac"].as<JsonString>().size() != 17) {
      fail(400, "STATION_INVALID"); return;
    }
    const auto id = doc["id"].as<unsigned>();
    const char* mac = doc["mac"];
    {
      Lock lock;
      if (!bleWorkerAvailable.load() || occupied(false) || blePaused || restartPending || portalRequested) {
        fail(409, "BLE_BUSY"); return;
      }
      for (unsigned i = 0; i < kStations; ++i) {
        if (configs[i].id == id && !strcasecmp(configs[i].mac, mac)) {
          identifySlot = i;
          break;
        }
      }
      if (identifySlot < 0) { fail(404, "STATION_INVALID"); return; }
      if (++identifyJob == 0) ++identifyJob;
      identifyResult = nullptr;
    }
    doc.clear();
    doc["job"] = identifyJob;
    sendJson(202, doc);
  });
  server.on("/api/identify", HTTP_GET, [] {
    if (!authenticated(true)) return;
    StaticJsonDocument<128> doc;
    {
      Lock lock;
      doc["job"] = identifyJob;
      doc["pending"] = identifyResult == nullptr;
      doc["error"] = identifyResult;
    }
    sendJson(200, doc);
  });
}
