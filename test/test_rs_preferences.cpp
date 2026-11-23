#include "web_account.h"
#include "rs_matter_commands.h"
#include "config_stream.h"
#include <cassert>
#include <cstdio>
#include <string>
using namespace bridge;
struct TextSink { std::string text; size_t write(const uint8_t* p,size_t n){text.append(reinterpret_cast<const char*>(p),n);return n;} };
int main(){
 for(const char* s:{"owner","VR_1","a.b-c","123"})assert(validWebUsername(s));
 for(const char* s:{"","admin user","<script>","a/b","a\n","ä","012345678901234567890123456789012"})assert(!validWebUsername(s));
 StaticJsonDocument<512> auth;bool enabled=false;const char* username=nullptr;
 assert(!std::strlen(storedWebUsername(auth.as<JsonVariantConst>())));
 auth["username"]=nullptr;assert(!std::strlen(storedWebUsername(auth.as<JsonVariantConst>())));
 auth["username"]=123;assert(!std::strlen(storedWebUsername(auth.as<JsonVariantConst>())));
 auth["username"]="owner";assert(!std::strcmp(storedWebUsername(auth.as<JsonVariantConst>()),"owner"));
 auth.clear();
 for(const char* s:{"{}","{\"username_enabled\":1}","{\"username_enabled\":true}","{\"username_enabled\":true,\"username\":\" \"}","{\"username_enabled\":false,\"username\":123}"}){assert(!deserializeJson(auth,s));assert(!parseWebUsername(auth.as<JsonVariantConst>(),enabled,username));}
 assert(!deserializeJson(auth,"{\"username_enabled\":false}"));assert(parseWebUsername(auth.as<JsonVariantConst>(),enabled,username)&&!enabled&&!std::strlen(username));
 auth["username_enabled"]=true;auth["username"]="owner";assert(parseWebUsername(auth.as<JsonVariantConst>(),enabled,username)&&enabled&&!std::strcmp(username,"owner"));
 StationConfig configs[kStations]{},next[kStations]{};StationState states[kStations]{};uint32_t counter=1;std::string error;
 DynamicJsonDocument doc(kStationJsonCapacity);auto array=doc.to<JsonArray>();
 for(unsigned i=0;i<kStations;++i){auto item=array.createNestedObject();item["id"]=0;item["name"]=std::string(37,'a')+std::to_string(100+i);char mac[18];std::snprintf(mac,sizeof(mac),"FA:00:00:00:00:%02X",i);item["mac"]=std::string(mac);item["alexa_compat"]=false;item["off_state"]=i%2?"standby":"sleep";}
 assert(!doc.overflowed());assert(parseStationList(array,configs,nullptr,counter,false,false,error));
 for(unsigned i=0;i<kStations;++i)assert(offTarget(configs[i])==(i%2?Target::Standby:Target::Sleep));
 TextSink sink;StatusWriter<TextSink> writer(sink);assert(streamConfiguration(writer,configs,counter));
 doc.clear();assert(!deserializeJson(doc,sink.text));assert(parseStationList(doc["stations"],next,nullptr,counter,true,false,error));
 for(unsigned i=0;i<kStations;++i){assert(next[i].offState==configs[i].offState&&next[i].hueId==configs[i].hueId);doc["stations"][i].remove("off_state");}
 assert(parseStationList(doc["stations"],next,configs,counter,false,false,error));assert(next[1].offState==Target::Standby);
 assert(parseStationList(doc["stations"],next,nullptr,counter,true,false,error));for(const auto& c:next)assert(c.offState==Target::Sleep);
 for(const char* bad:{"on","off","Standby","invalid"}){doc["stations"][0]["off_state"]=bad;assert(!parseStationList(doc["stations"],next,configs,counter,false,false,error)&&error=="CONFIG_OFF_STATE");}
 doc["stations"][0]["off_state"]=2;assert(!parseStationList(doc["stations"],next,configs,counter,false,false,error));
 MatterRegistry registry;assert(seedMatterRegistry(configs,registry,error));
 for(unsigned i=0;i<kStations;++i){assert(!states[i].hasRequest);auto accepted=acceptRsMatterCommand(configs,states,registry,registry.entries[i].endpoint,0,100,true,false);assert(accepted.station==i+1&&!accepted.on&&states[i].requested==configs[i].offState);const auto generation=states[i].generation;acceptRsMatterCommand(configs,states,registry,registry.entries[i].endpoint,0,200,true,false);assert(states[i].generation==generation&&states[i].requestedMs==100);states[i].pending=false;states[i].busy=true;finish(states[i],generation,true,i%2?0:2,300,"");assert(!states[i].commandConfirmed);recordRead(states[i],i%2?2:0,400,"");assert(states[i].commandConfirmed);assert(states[i].observed==(i%2?2:0));}
 Target target=Target::On;assert(!parseTarget("standby",target));assert(parseTarget("standby",target,true)&&target==Target::Standby);assert(matchesTarget(Target::Standby,2)&&!matchesTarget(Target::Standby,0)&&!matchesTarget(Target::Sleep,2)&&!matchesTarget(Target::On,1));
 puts("rs preferences: optional username validation, 16-station OFF persistence/migration, Matter OFF dispatch and exact STANDBY confirmation passed.");
}
