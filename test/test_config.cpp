#include "station_config.h"
#include <cassert>
#include <iostream>
using namespace bridge;
int main() {
 StationConfig old[kStations],next[kStations],restored[kStations];uint32_t counter=6;std::string error;
 DynamicJsonDocument doc(8192);
 // Old four-slot records: discard empty slots, retain original occupied slots and Hue IDs.
 assert(!deserializeJson(doc,R"([{"name":"First","mac":"AA:BB:CC:DD:EE:01"},{"name":"Empty","mac":""},{"name":"Third","mac":"AA:BB:CC:DD:EE:03"},{"name":"Fourth","mac":"AA:BB:CC:DD:EE:04"}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),old,nullptr,counter,true,true,error));
 assert(old[0].id==1&&old[1].id==3&&old[2].id==4&&!old[3].id);
 assert(old[0].hueId==1&&old[1].hueId==3&&old[2].hueId==4&&counter==6);
 // Owner's pre-0.3.6 configuration: local IDs become 1..4 without changing Alexa.
 assert(!deserializeJson(doc,R"([{"id":6,"name":"A","mac":"AA:BB:CC:DD:EE:06"},{"id":7,"name":"B","mac":"AA:BB:CC:DD:EE:07"},{"id":8,"name":"C","mac":"AA:BB:CC:DD:EE:08"},{"id":9,"name":"D","mac":"AA:BB:CC:DD:EE:09"}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),old,nullptr,counter,true,false,error));
 for(unsigned i=0;i<4;++i){assert(old[i].id==i+1&&old[i].hueId==i+6&&old[i].addressType==1);assert(stationIdForHue(old,i+6)==static_cast<int>(i+1));}
 assert(counter==10&&!stationIdForHue(old,5)&&!stationIdForHue(old,999));
 doc.clear();writeStationList(doc.to<JsonArray>(),old,true);
 assert(parseStationList(doc.as<JsonVariantConst>(),restored,nullptr,counter,true,false,error));
 for(unsigned i=0;i<4;++i)assert(restored[i].id==i+1&&restored[i].hueId==i+6);
 doc.clear();writeStationList(doc.to<JsonArray>(),old);assert(!doc[0].containsKey("hue_id"));
 // Remove B, reorder C/A, append a new entry: reuse slot 2 without changing A/C.
 assert(!deserializeJson(doc,R"([{"id":3,"name":"C","mac":"AA:BB:CC:DD:EE:08","hue_id":999},{"id":1,"name":"A renamed","mac":"AA:BB:CC:DD:EE:06"},{"id":0,"name":"New","mac":""}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),next,old,counter,false,false,error));
 assert(next[0].id==1&&next[0].hueId==6&&next[1].id==2&&next[1].hueId==10&&next[2].id==3&&next[2].hueId==8&&counter==11);
 StationState states[kStations],reconciled[kStations];recordRead(states[0],11,1000,"");states[0].commandConfirmed=true;states[0].generation=13;
 recordRead(states[2],0,1100,"");states[2].generation=21;
 reconcileStationStates(next,old,states,reconciled);
 assert(reconciled[0].observed==11&&reconciled[0].generation==14&&reconciled[0].commandConfirmed);
 assert(reconciled[2].observed==0&&reconciled[1].observed==-1);
 finishRead(reconciled[0],13,11,2000,"",0);assert(reconciled[0].observed==11);
 next[0].mac[16]='9';reconcileStationStates(next,old,states,reconciled);assert(reconciled[0].observed==-1&&!reconciled[0].commandConfirmed);
 // A deleted first/middle slot does not renumber survivors, including after reboot.
 assert(!deserializeJson(doc,R"([{"id":3,"name":"C","mac":"AA:BB:CC:DD:EE:08"},{"id":4,"name":"D","mac":"AA:BB:CC:DD:EE:09"}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),next,old,counter,false,false,error));
 assert(next[0].id==3&&next[1].id==4);
 reconcileStationStates(next,old,states,reconciled);assert(reconciled[0].observed==0&&reconciled[0].generation==14);
 finishRead(reconciled[0],13,11,2000,"",0);assert(reconciled[0].observed==0);
 doc.clear();writeStationList(doc.to<JsonArray>(),next,true);
 assert(parseStationList(doc.as<JsonVariantConst>(),restored,nullptr,counter,true,false,error));
 assert(restored[0].id==3&&restored[1].id==4&&restored[0].hueId==8);
 // New entries before retained IDs cannot steal them; allocation skips occupied slots.
 assert(!deserializeJson(doc,R"([{"id":0,"name":"New","mac":""},{"id":1,"name":"A","mac":"AA:BB:CC:DD:EE:06"}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),next,old,counter,false,false,error));assert(next[0].id==1&&next[0].hueId==6&&next[1].id==2);
 // Profile defaults off, persists, survives omitted fields, and rejects invalid types.
 assert(!old[0].alexaCompat);
 assert(!deserializeJson(doc,R"([{"id":1,"name":"A","mac":"AA:BB:CC:DD:EE:06","alexa_compat":true}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),next,old,counter,false,false,error));assert(next[0].alexaCompat&&next[0].hueId==6);
 doc.clear();writeStationList(doc.to<JsonArray>(),next,true);
 assert(parseStationList(doc.as<JsonVariantConst>(),restored,nullptr,counter,true,false,error));assert(restored[0].alexaCompat&&restored[0].hueId==6);
 assert(!deserializeJson(doc,R"([{"id":1,"name":"A","mac":"AA:BB:CC:DD:EE:06"}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),next,restored,counter,false,false,error));assert(next[0].alexaCompat);
 doc[0]["alexa_compat"]=false;assert(parseStationList(doc.as<JsonVariantConst>(),next,restored,counter,false,false,error));assert(!next[0].alexaCompat);
 doc[0]["alexa_compat"]="yes";assert(!parseStationList(doc.as<JsonVariantConst>(),next,restored,counter,false,false,error)&&error=="CONFIG_PROFILE");
 // All 16 local IDs, including local station 5, are usable; Hue 5 stays retired.
 counter=1;doc.clear();auto list=doc.to<JsonArray>();
 for(unsigned i=0;i<16;++i){auto item=list.createNestedObject();item["id"]=0;item["name"]="Station "+std::to_string(i);item["mac"]="";}
 assert(parseStationList(list,next,old,counter,false,false,error));
 for(unsigned i=0;i<16;++i)assert(next[i].id==i+1&&next[i].hueId==(i<4?i+1:i+2));
 assert(next[4].id==5&&next[4].hueId==6&&counter==18);
 list.createNestedObject();assert(!parseStationList(list,next,old,counter,false,false,error)&&error=="CONFIG_LIMIT");
 for(auto invalid:{R"([{"id":123,"name":"X","mac":""}])",R"([{"id":-1,"name":"X","mac":""}])",R"([{"id":"1","name":"X","mac":""}])",R"([{"id":1,"name":"X","mac":""},{"id":1,"name":"Y","mac":""}])",R"([{"name":"X","mac":"aa:bb:cc:dd:ee:01"},{"name":"Y","mac":"AA:BB:CC:DD:EE:01"}])",R"([{"name":"X","mac":""},{"name":"X","mac":""}])"}){
  assert(!deserializeJson(doc,invalid));uint32_t trial=counter;assert(!parseStationList(doc.as<JsonVariantConst>(),next,old,trial,false,false,error));
 }
 assert(!deserializeJson(doc,R"([{"id":1,"hue_id":5,"name":"X","mac":""}])"));assert(!parseStationList(doc.as<JsonVariantConst>(),next,nullptr,counter,true,false,error));
 for(uint32_t mac:{0U,0x112233U,0xffffffU})for(uint32_t id:{1U,2U,3U,4U,6U,17U,123456U,kMaxStationId})assert(stationIdFromHue(mac,hueKey(mac,id))==id);
 for(auto valid:{"bridge","vr-room-2","a","123"})assert(validDeviceName(valid));
 for(auto bad:{"","Uppercase","-start","end-","two words","name.local"})assert(!validDeviceName(bad));
 std::cout<<"Fixed reusable 1..16 slots, legacy migration, stable Alexa identities, 16 entries and read-generation safety passed.\n";
}
