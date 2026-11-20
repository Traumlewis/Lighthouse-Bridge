#pragma once
#include <ArduinoJson.h>
#include <cstdint>
#include <cstring>

namespace bridge {
enum class DeviceAction { Restart, FactoryReset };
inline const char* parseDeviceAction(JsonVariantConst input, DeviceAction& action) {
  if (!input.is<JsonObjectConst>() || !input["action"].is<const char*>()) return "MAINTENANCE_INVALID";
  const char* value=input["action"];
  if(input["action"].as<JsonString>().size()!=std::strlen(value))return "MAINTENANCE_INVALID";
  if (!std::strcmp(value,"restart") && input.size()==1) { action=DeviceAction::Restart;return nullptr; }
  if (std::strcmp(value,"factory_reset")) return "MAINTENANCE_INVALID";
  if (!input["confirm"].is<bool>() || !input["confirm"].as<bool>()) return "RESET_CONFIRM_REQUIRED";
  if(input.size()!=2)return "MAINTENANCE_INVALID";
  action=DeviceAction::FactoryReset;return nullptr;
}
// A display label is data, not HTML or a DNS hostname. Bound UTF-8 bytes and
// reject control characters, overlong sequences, surrogates and invalid input.
inline bool validRoomName(const char* value) {
  if (!value) return false;
  const size_t size=std::strlen(value);
  if(!size||size>64)return false;
  bool visible=false;
  for(size_t i=0;i<size;){
    const uint8_t c=static_cast<uint8_t>(value[i++]);
    if(c<32||c==127)return false;
    if(c<128){if(c!=' ')visible=true;continue;}
    unsigned count;uint32_t code;
    if(c>=0xc2&&c<=0xdf){count=1;code=c&31;}
    else if(c>=0xe0&&c<=0xef){count=2;code=c&15;}
    else if(c>=0xf0&&c<=0xf4){count=3;code=c&7;}
    else return false;
    if(i+count>size)return false;
    for(unsigned j=0;j<count;++j){const uint8_t next=static_cast<uint8_t>(value[i++]);if((next&0xc0)!=0x80)return false;code=(code<<6)|(next&63);}
    if((count==1&&code<128)||(count==2&&code<0x800)||(count==3&&code<0x10000)||
       (code>=0xd800&&code<=0xdfff)||code>0x10ffff||(code>=0x80&&code<=0x9f))return false;
    visible=true;
  }
  return visible;
}
// The reset request is durable before scheduling a reboot. No erase happens in
// a running HTTP/BLE/Matter process, where other tasks could repopulate storage.
template<class Storage> bool prepareDeviceAction(DeviceAction action,Storage& storage) {
  return action!=DeviceAction::FactoryReset || storage.markFactoryReset();
}
template<class Storage> bool applyFactoryResetAtBoot(Storage& storage) {
  bool requested=false;
  if(!storage.resetRequested(requested))return false;
  if(!requested)return true;
  return storage.deinitialize() && storage.erase() && storage.initialize();
}
inline bool maintenanceRestartReady(uint32_t now,uint32_t accepted,bool requireIdle,bool bleActive) {
  return now-accepted>=1500 && (!requireIdle || !bleActive);
}
} // namespace bridge
