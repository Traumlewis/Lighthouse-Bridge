#pragma once
#include <nvs.h>
#include <nvs_flash.h>
#include "device_maintenance.h"

// Run before Preferences, Wi-Fi, BLE or Rust initialization. Only the confirmed
// request sets this flag. Erase settings/fabrics together, never app/OTA slots.
struct FactoryResetStorage {
  bool resetRequested(bool& requested) {
    nvs_handle_t handle;
    esp_err_t result=nvs_open("lhbridge",NVS_READONLY,&handle);
    if(result==ESP_ERR_NVS_NOT_FOUND){requested=false;return true;}
    if(result!=ESP_OK)return false;
    uint8_t value=0;result=nvs_get_u8(handle,"factory_reset",&value);nvs_close(handle);
    if(result==ESP_ERR_NVS_NOT_FOUND){requested=false;return true;}
    requested=result==ESP_OK&&value==1;
    return result==ESP_OK&&value<=1;
  }
  bool deinitialize(){const auto result=nvs_flash_deinit();return result==ESP_OK||result==ESP_ERR_NVS_NOT_INITIALIZED;}
  bool erase(){return nvs_flash_erase()==ESP_OK;}
  bool initialize(){return nvs_flash_init()==ESP_OK;}
};
struct MaintenanceFlagStorage {
  bool markFactoryReset(){return prefs.putBool("factory_reset",true)==sizeof(uint8_t);}
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
    StaticJsonDocument<128> doc;
    if(!readBody(doc))return;
    DeviceAction action=DeviceAction::Restart;
    const char* error=parseDeviceAction(doc.as<JsonVariantConst>(),action);
    if(error){fail(400,error);return;}
    {
      Lock lock;
      if(occupied(false)||blePaused||otaPrepared||restartPending||portalRequested){fail(409,"BLE_BUSY");return;}
      MaintenanceFlagStorage storage;
      if(!prepareDeviceAction(action,storage)){fail(500,"SAVE_FAILED");return;}
      blePaused=true;maintenanceWaitIdle=true;restartPending=true;restartAt=millis();
    }
    doc.clear();doc["accepted"]=true;doc["restarting"]=true;sendJson(202,doc);
    restartAt=millis();
  });
}
