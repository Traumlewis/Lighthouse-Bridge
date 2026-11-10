#include "station_config.h"
#include <cassert>
#include <iostream>
using namespace bridge;
int main() {
 StationConfig old[kStations], next[kStations]; uint32_t counter=6; std::string error;
 DynamicJsonDocument doc(8192);
 assert(!deserializeJson(doc,R"([{"name":"First","mac":"AA:BB:CC:DD:EE:01","address_type":0},{"name":"Empty","mac":""},{"name":"Third","mac":"AA:BB:CC:DD:EE:03","address_type":1},{"name":"Fourth","mac":"AA:BB:CC:DD:EE:04"}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),old,nullptr,counter,true,true,error));
 assert(old[0].id==1 && old[1].id==3 && old[2].id==4 && !old[3].id);
 assert(old[0].addressType==1 && counter==6);
 doc.clear(); auto list=doc.to<JsonArray>();
 for(unsigned i=0;i<16;++i){auto item=list.createNestedObject();item["id"]=0;item["name"]="Station "+std::to_string(i);item["mac"]="";}
 assert(parseStationList(list,next,old,counter,false,false,error));
 assert(next[0].id==6 && next[15].id==21 && counter==22);
 list.createNestedObject();assert(!parseStationList(list,next,old,counter,false,false,error));assert(error=="CONFIG_LIMIT");
 // Reloading a saved record preserves stable IDs and never lowers its counter.
 counter=6;doc.clear();writeStationList(doc.to<JsonArray>(),old);
 assert(parseStationList(doc.as<JsonVariantConst>(),next,nullptr,counter,true,false,error));
 assert(next[1].id==3 && counter==6);
 // Removal/reordering does not rename IDs; added devices never reuse removed ones.
 assert(!deserializeJson(doc,R"([{"id":4,"name":"Fourth","mac":"AA:BB:CC:DD:EE:04"},{"id":0,"name":"New","mac":""},{"id":1,"name":"First renamed","mac":"AA:BB:CC:DD:EE:01"}])"));
 assert(parseStationList(doc.as<JsonVariantConst>(),next,old,counter,false,false,error));assert(next[0].id==4&&next[1].id==6&&next[2].id==1&&counter==7);
 StationState states[kStations], reconciled[kStations];
 recordRead(states[0],11,1000,"");states[0].commandConfirmed=true;states[0].generation=13;
 recordRead(states[2],0,1100,"");states[2].generation=21;
 reconcileStationStates(next,old,states,reconciled);
 assert(reconciled[0].observed==0 && reconciled[0].generation==14);
 assert(reconciled[2].observed==11 && reconciled[2].commandConfirmed && reconciled[2].generation==22);
 finishRead(reconciled[0],13,11,2000,"",0);assert(reconciled[0].observed==0);
 next[2].mac[16]='9';reconcileStationStates(next,old,states,reconciled);assert(reconciled[2].observed==-1&&!reconciled[2].commandConfirmed);
 for(auto invalid : {R"([{"id":5,"name":"X","mac":""}])",R"([{"id":123,"name":"X","mac":""}])",R"([{"id":-1,"name":"X","mac":""}])",R"([{"id":"1","name":"X","mac":""}])",R"([{"id":1,"name":"X","mac":""},{"id":1,"name":"Y","mac":""}])",R"([{"name":"X","mac":"aa:bb:cc:dd:ee:01"},{"name":"Y","mac":"AA:BB:CC:DD:EE:01"}])",R"([{"name":"X","mac":""},{"name":"X","mac":""}])"}) {
  assert(!deserializeJson(doc,invalid));uint32_t trial=counter;assert(!parseStationList(doc.as<JsonVariantConst>(),next,old,trial,false,false,error));
 }
 for(uint32_t mac:{0U,0x112233U,0xffffffU}) for(uint32_t id:{1U,2U,3U,4U,6U,17U,123456U,kMaxStationId})assert(stationIdFromHue(mac,hueKey(mac,id))==id);
 assert(!stationIdFromHue(0x112233,hueKey(0x112233,5)));
 for(auto valid:{"bridge","vr-room-2","a","123"})assert(validDeviceName(valid));
 for(auto bad:{"","Uppercase","-start","end-","two words","name.local","abcdefghijklmnopqrstuvwxyz1234567"})assert(!validDeviceName(bad));
 std::cout<<"Station migration, 16-entry capacity, stable IDs, retired group, validation and hostname rules passed.\n";
}
