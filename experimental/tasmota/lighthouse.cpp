// GPL-3.0-or-later when linked with the pinned Tasmota distribution.
#define LH_MATTER_BUILD 1
#define LH_TASMOTA_BUILD 1
#define LH_FIRMWARE_VERSION "0.1.4-tasmota"
#define setup lighthouseSetup
#define loop lighthouseLoop
#include "tasmota_runtime.h"
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "main.cpp"
