#include "network_mode.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
using namespace bridge;
struct Store {
 std::map<std::string,std::string> data{{"web_auth","saved auth"},{"access_key","saved key"},{"stations","saved stations"},{"router_wifi","saved wifi"},{"fabric","saved fabric"}};
 bool fail=false;unsigned writes=0;
 bool getBool(const char* key,bool fallback){const auto i=data.find(key);return i==data.end()?fallback:i->second=="1";}
 size_t putBool(const char* key,bool value){++writes;assert(!std::strcmp(key,"wifi_setup"));if(fail)return 0;data[key]=value?"1":"0";return 1;}
 size_t putString(const char* key,const char* value){++writes;assert(!std::strcmp(key,"wifi_mode"));if(fail)return 0;data[key]=value;return std::strlen(value);}
};
int main(){
 assert(storedNetworkMode(nullptr)==NetworkMode::Home);
 for(const char* value:{"","home","invalid","LOCAL","local "})assert(storedNetworkMode(value)==NetworkMode::Home);
 assert(storedNetworkMode("local")==NetworkMode::Local);
 Store store;const auto retained=store.data;
 assert(saveNetworkMode(store,NetworkMode::Local));assert(storedNetworkMode(store.data["wifi_mode"].c_str())==NetworkMode::Local);
 store.fail=true;assert(!saveNetworkMode(store,NetworkMode::Home));assert(store.data["wifi_mode"]=="local");
 store.fail=false;assert(saveNetworkMode(store,NetworkMode::Home));assert(store.data["wifi_mode"]=="home");
 for(const auto& saved:retained)assert(store.data[saved.first]==saved.second);
 assert(store.writes==3);
 bool setup=true;assert(consumeWifiSetup(store,setup)&&!setup&&store.writes==3);
 assert(requestWifiSetup(store));assert(store.data["wifi_setup"]=="1"&&store.data["wifi_mode"]=="home");
 assert(consumeWifiSetup(store,setup)&&setup&&store.data["wifi_setup"]=="0");
 assert(consumeWifiSetup(store,setup)&&!setup); // Once only, including after the setup timeout/reboot.
 store.fail=true;assert(!requestWifiSetup(store)&&store.data["wifi_setup"]=="0");
 store.fail=false;assert(requestWifiSetup(store));store.fail=true;assert(!consumeWifiSetup(store,setup)&&!setup&&store.data["wifi_setup"]=="1");
 store.fail=false;assert(consumeWifiSetup(store,setup)&&setup);assert(store.data["wifi_mode"]=="home");
 for(const auto& saved:retained)assert(store.data[saved.first]==saved.second);
 puts("Network mode: legacy home default, exact local selection, failed persistence, one-shot setup consumption/failure and unrelated settings retained.");
}
