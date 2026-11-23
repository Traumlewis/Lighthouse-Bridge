#pragma once
#include <nvs.h>
#include <nvs_flash.h>
#include "device_maintenance.h"

// Run before Preferences, Wi-Fi, BLE or Rust initialization. Only the confirmed
// request sets this flag. Erase settings/fabrics together, never app/OTA slots.
struct FactoryResetStorage {
  char nextKey[25]{};
  bool resetRequested(bool& requested) {
    nvs_handle_t handle;
    esp_err_t result=nvs_open("lhbridge",NVS_READONLY,&handle);
    if(result==ESP_ERR_NVS_NOT_FOUND){requested=false;return true;}
    if(result!=ESP_OK)return false;
    size_t keySize = sizeof nextKey;
    result = nvs_get_str(handle, "reset_next_key", nextKey, &keySize);
    if (result != ESP_ERR_NVS_NOT_FOUND) {
      nvs_close(handle);
      requested = result == ESP_OK && keySize == sizeof nextKey && validResetKey(nextKey);
      return requested;
    }
    uint8_t value=0;result=nvs_get_u8(handle,"factory_reset",&value);nvs_close(handle);
    if(result==ESP_ERR_NVS_NOT_FOUND){requested=false;return true;}
    requested=result==ESP_OK&&value==1;
    return result==ESP_OK&&value<=1;
  }
  bool deinitialize(){const auto result=nvs_flash_deinit();return result==ESP_OK||result==ESP_ERR_NVS_NOT_INITIALIZED;}
  bool erase(){return nvs_flash_erase()==ESP_OK;}
  bool initialize() {
    if (nvs_flash_init()!=ESP_OK) return false;
    if (!nextKey[0]) return true; // Legacy reset flag: generate a key at normal startup.
    nvs_handle_t handle;
    if (nvs_open("lhbridge", NVS_READWRITE, &handle)!=ESP_OK) return false;
    const bool saved = nvs_set_str(handle, "access_key", nextKey)==ESP_OK && nvs_commit(handle)==ESP_OK;
    nvs_close(handle);
    return saved;
  }
};
String resetPreviewKey, resetPreviewOwner;
uint32_t resetPreviewAt = 0;
struct MaintenanceFlagStorage {
  bool markFactoryReset() {
    return prefs.putString("reset_next_key", resetPreviewKey)==24;
  }
};
void configureRsMaintenanceRoutes() {
  server.on("/api/room",HTTP_POST,[]{
    if(!authenticated(true)||!sameOriginJson())return;
    if(otaPrepared||restartPending||portalRequested){fail(409,"BLE_BUSY");return;}
    StaticJsonDocument<256> doc;
    if(!readBody(doc))return;
    if(!doc.is<JsonObject>()||doc.size()!=1||!doc["name"].is<const char*>()||
       doc["name"].as<JsonString>().size()!=std::strlen(doc["name"].as<const char*>())||!validRoomName(doc["name"])){
      fail(400,"ROOM_NAME_INVALID");return;
    }
    const String value=doc["name"].as<String>();
    if(prefs.putString("room_name",value)!=value.length()){fail(500,"SAVE_FAILED");return;}
    roomName=value;doc.clear();doc["saved"]=true;doc["name"]=roomName;sendJson(200,doc);
  });
  server.on("/api/maintenance",HTTP_POST,[]{
    if(!authenticated(true)||!sameOriginJson())return;
    StaticJsonDocument<256> doc;
    if(!readBody(doc))return;
    DeviceAction action=DeviceAction::Restart;
    const char* error=parseDeviceAction(doc.as<JsonVariantConst>(),action);
    if(error){fail(400,error);return;}
    if (action == DeviceAction::FactoryPreview) {
      if (restartPending || otaPrepared || portalRequested) { fail(409,"BLE_BUSY"); return; }
      resetPreviewKey = randomHex(12);
      resetPreviewOwner = sessions[sessionIndex()].token;
      resetPreviewAt = millis();
      if (!validResetKey(resetPreviewKey.c_str()) || equalSecret(resetPreviewKey, accessKey) ||
          !resetPreviewOwner.length()) { fail(500,"SAVE_FAILED"); return; }
      doc.clear();
      doc["reset_key"] = resetPreviewKey;
      sendJson(200,doc);
      return;
    }
    if (action == DeviceAction::FactoryReset &&
        (!resetPreviewFresh(millis(), resetPreviewAt) ||
         !equalSecret(resetPreviewOwner, sessions[sessionIndex()].token) ||
         !equalSecret(resetPreviewKey, doc["reset_key"].as<String>()) ||
         equalSecret(resetPreviewKey, accessKey))) {
      fail(409,"RESET_PREVIEW_REQUIRED"); return;
    }
    {
      Lock lock;
      if(occupied(false)||blePaused||otaPrepared||restartPending||portalRequested){fail(409,"BLE_BUSY");return;}
      MaintenanceFlagStorage storage;
      if(!prepareDeviceAction(action,storage)){fail(500,"SAVE_FAILED");return;}
      blePaused=true;maintenanceWaitIdle=true;restartPending=true;restartAt=millis();
    }
    resetPreviewKey = "";
    resetPreviewOwner = "";
    doc.clear();doc["accepted"]=true;doc["restarting"]=true;sendJson(202,doc);
    restartAt=millis();
  });
}
