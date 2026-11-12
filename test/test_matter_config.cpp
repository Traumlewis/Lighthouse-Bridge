#include "matter_config.h"
#include <cassert>
#include <cstdio>
#include <iostream>
using namespace bridge;
static StationConfig config(unsigned id, unsigned hue, unsigned address) {
  StationConfig value; value.id=id; value.hueId=hue;
  std::snprintf(value.name,sizeof(value.name),"Station %u",id);
  std::snprintf(value.mac,sizeof(value.mac),"02:00:00:00:00:%02X",address);
  return value;
}
static unsigned endpoint(const MatterRegistry& map,const StationConfig& value) {
  const int index=matterBindingIndex(map,value.hueId,value.mac); assert(index>=0);
  return map.entries[index].endpoint;
}
int main() {
  StationConfig old[kStations]{},next[kStations]{};
  old[0]=config(1,6,1);old[1]=config(2,7,2);old[2]=config(4,9,4);
  MatterRegistry initial,planned,reloaded;std::string error;
  assert(seedMatterRegistry(old,initial,error));
  assert(endpoint(initial,old[0])==1);assert(endpoint(initial,old[1])==3);assert(endpoint(initial,old[2])==4);
  next[0]=old[2];next[1]=old[0];next[2]=old[1];
  assert(planMatterRegistry(old,next,initial,planned,error));
  assert(endpoint(planned,old[0])==1&&endpoint(planned,old[2])==4&&planned.nextEndpoint==initial.nextEndpoint);
  next[0]=old[0];next[1]=old[1];next[2]=old[2];
  next[0]=config(1,6,99); // Same local/Hue IDs, different physical device.
  assert(planMatterRegistry(old,next,initial,planned,error));
  assert(endpoint(planned,old[0])==1&&endpoint(planned,next[0])==5&&planned.nextEndpoint==6);
  // Failure after checkpointing keeps old survivors, without reusing wasted IDs.
  MatterRegistry interrupted;
  assert(planMatterRegistry(old,old,planned,interrupted,error));
  assert(endpoint(interrupted,old[0])==1&&interrupted.nextEndpoint==6);
  // A later return to a retired MAC is new work, not resurrection of endpoint 1.
  MatterRegistry returned;
  assert(planMatterRegistry(next,old,planned,returned,error));
  assert(endpoint(returned,old[0])==6&&endpoint(returned,next[0])==5);
  DynamicJsonDocument json(8192);writeMatterRegistry(json.to<JsonObject>(),planned);
  assert(parseMatterRegistry(json.as<JsonVariantConst>(),reloaded,error));
  assert(reloaded.nextEndpoint==planned.nextEndpoint&&endpoint(reloaded,next[0])==5);
  json["bindings"][1]["endpoint"]=json["bindings"][0]["endpoint"];
  assert(!parseMatterRegistry(json.as<JsonVariantConst>(),reloaded,error));
  json.clear();writeMatterRegistry(json.to<JsonObject>(),planned);json["version"]="1";
  assert(!parseMatterRegistry(json.as<JsonVariantConst>(),reloaded,error));
  json.clear();writeMatterRegistry(json.to<JsonObject>(),planned);json["bindings"][0]["endpoint"]=2;
  assert(!parseMatterRegistry(json.as<JsonVariantConst>(),reloaded,error));
  for(unsigned i=0;i<kStations;++i){old[i]=config(i+1,i<4?i+1:i+2,i+1);next[i]=config(i+1,100+i,100+i);}
  assert(seedMatterRegistry(old,initial,error));
  assert(planMatterRegistry(old,next,initial,planned,error));
  assert(planned.count==32);
  for(unsigned i=0;i<kStations;++i){assert(endpoint(planned,old[i])==endpoint(initial,old[i]));assert(endpoint(planned,next[i])>=initial.nextEndpoint);}
  assert(planMatterRegistry(next,next,planned,reloaded,error));assert(reloaded.count==16);
  for(unsigned i=0;i<kStations;++i)assert(endpoint(reloaded,next[i])==endpoint(planned,next[i]));
  initial.nextEndpoint=kMatterEndpointLimit+1;
  assert(!planMatterRegistry(old,next,initial,planned,error));assert(error=="MATTER_IDS_EXHAUSTED");
  std::cout<<"Matter identities passed: prototype migration, 16 stations, reorder/replacement, interrupted saves and exhaustion.\n";
}
