#include "device_maintenance.h"
#include <cassert>
#include <cstdio>
#include <string>
using namespace bridge;
struct Storage {
  bool requested=false,readOK=true,markOK=true,deinitOK=true,eraseOK=true,initOK=true;
  std::string calls,settings="wifi,key,auth,room,stations,buttons,fabric",apps="two original app slots",ota="boot selection";
  bool markFactoryReset(){calls+='m';if(markOK)requested=true;return markOK;}
  bool resetRequested(bool& flag){calls+='r';flag=requested;return readOK;}
  bool deinitialize(){calls+='d';return deinitOK;}
  bool erase(){calls+='e';if(eraseOK){settings.clear();requested=false;}return eraseOK;}
  bool initialize(){calls+='i';return initOK;}
};
int main(){
  StaticJsonDocument<512> doc;DeviceAction action=DeviceAction::Restart;
  for(const char* body:{"null","[]","{}","{\"action\":1}","{\"action\":\"Restart\"}","{\"action\":\"restart\",\"extra\":1}","{\"action\":\"factory_reset\"}","{\"action\":\"factory_reset\",\"confirm\":false}","{\"action\":\"factory_reset\",\"confirm\":1}","{\"action\":\"factory_reset\",\"confirm\":true,\"extra\":1}","{\"action\":\"restart\\u0000reset\"}"}){
    assert(!deserializeJson(doc,body));assert(parseDeviceAction(doc.as<JsonVariantConst>(),action));
  }
  assert(!deserializeJson(doc,"{\"action\":\"restart\"}"));assert(!parseDeviceAction(doc.as<JsonVariantConst>(),action)&&action==DeviceAction::Restart);
  Storage normal;assert(prepareDeviceAction(action,normal)&&normal.calls.empty());assert(applyFactoryResetAtBoot(normal)&&normal.calls=="r"&&!normal.settings.empty());
  assert(!deserializeJson(doc,R"({"action":"factory_preview"})"));
  assert(!parseDeviceAction(doc.as<JsonVariantConst>(),action)&&action==DeviceAction::FactoryPreview);
  assert(prepareDeviceAction(action,normal)&&normal.calls=="r");
  assert(!deserializeJson(doc,R"({"action":"factory_reset","confirm":true})"));
  assert(!strcmp(parseDeviceAction(doc.as<JsonVariantConst>(),action),"RESET_PREVIEW_REQUIRED"));
  assert(!deserializeJson(doc,R"({"action":"factory_reset","confirm":true,"reset_key":"0123456789abcdef01234567"})"));
  assert(!parseDeviceAction(doc.as<JsonVariantConst>(),action)&&action==DeviceAction::FactoryReset);
  assert(!validResetKey("0123456789abcDef01234567")&&!validResetKey("old"));
  assert(resetPreviewFresh(299999,0)&&!resetPreviewFresh(300000,0));
  assert(resetPreviewFresh(10,UINT32_MAX-10));
  Storage reset;assert(prepareDeviceAction(action,reset)&&reset.calls=="m"&&reset.requested&&!reset.settings.empty());
  assert(applyFactoryResetAtBoot(reset)&&reset.calls=="mrdei"&&reset.settings.empty()&&!reset.requested);
  assert(reset.apps=="two original app slots"&&reset.ota=="boot selection");reset.calls.clear();assert(applyFactoryResetAtBoot(reset)&&reset.calls=="r");
  Storage failed;failed.markOK=false;assert(!prepareDeviceAction(action,failed)&&!failed.requested&&!failed.settings.empty());
  for(unsigned phase=0;phase<4;++phase){Storage s;s.requested=true;if(phase==0)s.readOK=false;if(phase==1)s.deinitOK=false;if(phase==2)s.eraseOK=false;if(phase==3)s.initOK=false;assert(!applyFactoryResetAtBoot(s));assert(s.calls==std::string("rdei").substr(0,phase+1));assert(s.apps=="two original app slots"&&s.ota=="boot selection");}
  assert(!maintenanceRestartReady(1499,0,true,false));assert(!maintenanceRestartReady(5000,0,true,true));assert(maintenanceRestartReady(1500,0,true,false));assert(maintenanceRestartReady(1500,0,false,true));assert(maintenanceRestartReady(1000,UINT32_MAX-500,true,false));
  for(const char* name:{"My VR room","Leas VR-Raum","Änne 🎮","<owner> & \"room\""})assert(validRoomName(name));
  assert(validRoomName(std::string(64,'a').c_str()));assert(!validRoomName(std::string(65,'a').c_str()));assert(validRoomName(std::string(32*2,'a').c_str()));
  for(const char* name:{"","   ","room\n","room\x7f","\x80","\xc0\xaf","\xe0\x80\xaf","\xed\xa0\x80","\xf4\x90\x80\x80","\xf0\x9f","\xc2\x85"})assert(!validRoomName(name));
  assert(!validRoomName(nullptr));
  puts("Maintenance: strict confirmation, durable flag, boot erase ordering/failures, app preservation, idle delay/wrap and bounded UTF-8 names passed.");
}
