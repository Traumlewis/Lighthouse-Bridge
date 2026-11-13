#include "matter_endpoint.h"
#include <map>
#include <cassert>
#include <iostream>
struct FakeSdk {
  struct Endpoint { uint16_t id; void* data; bool enabled; };
  using Handle = Endpoint*;
  uint32_t next = 1, internalNext = 1;
  bool started = false, failCreate = false, failResume = false, badCounter = false;
  unsigned creates = 0, resumes = 0, maxLive = 1;
  std::map<uint16_t,Endpoint> endpoints;
  FakeSdk() { endpoints.emplace(0,Endpoint{0,nullptr,false}); }
  void begin(uint32_t persisted) { started = true; next = internalNext = persisted; }
  Handle find(uint16_t id) { const auto it=endpoints.find(id);return it==endpoints.end()?nullptr:&it->second; }
  Handle resume(uint16_t id,uint8_t,void* data) {
    ++resumes;
    if (failResume || id>=internalNext || find(id)) return nullptr;
    return add(id,data);
  }
  Handle create(uint8_t,void* data) {
    ++creates;
    if (failCreate || internalNext >= 0xffff) return nullptr;
    const uint16_t id=static_cast<uint16_t>(internalNext++ + (badCounter?1:0));
    return add(id,data);
  }
  Handle add(uint16_t id,void* data) {
    auto it=endpoints.emplace(id,Endpoint{id,data,false}).first;
    if(endpoints.size()>maxLive)maxLive=endpoints.size();
    return &it->second;
  }
  uint16_t id(Handle p) {return p->id;}
  bool destroy(Handle p) {assert(!p->enabled);endpoints.erase(p->id);return true;}
  void yield() {}
};
int main(){
  FakeSdk old;
  assert(!old.resume(2,0,nullptr)); // Reproduce the pre-start 0.2/0.3.1 failure.
  for(uint32_t persisted : {1U,3U,20U}){
    FakeSdk sdk;sdk.begin(persisted);
    int privateData=7;
    auto parent=bridge::stableMatterEndpoint(sdk,2,0,nullptr);assert(parent&&parent->id==2);
    for(uint16_t id : {1,3,4,5,9,21,22}){
      auto lamp=bridge::stableMatterEndpoint(sdk,id,2,&privateData);
      assert(lamp&&lamp->id==id&&lamp->data==&privateData);lamp->enabled=true;
    }
    assert(sdk.endpoints.size()==9); // Only root, aggregate and seven actual lights.
    assert(!sdk.find(6)&&!sdk.find(20));
    assert(sdk.maxLive<=10); // At most one empty, never-enabled placeholder.
    assert(sdk.next==sdk.internalNext);
    const auto count=sdk.creates;
    assert(!bridge::stableMatterEndpoint(sdk,2,0,nullptr));assert(sdk.creates==count);
  }
  FakeSdk full;full.begin(3);
  assert(bridge::stableMatterEndpoint(full,2,0,nullptr));
  for(uint16_t id=3;id<=18;++id)assert(bridge::stableMatterEndpoint(full,id,2,nullptr));
  assert(full.endpoints.size()==18);
  FakeSdk exhausted;exhausted.begin(0xffff);
  assert(bridge::stableMatterEndpoint(exhausted,0xfffe,2,nullptr));
  assert(exhausted.creates==0&&exhausted.next==0xffff); // No counter wrap/probe.
  assert(!bridge::stableMatterEndpoint(exhausted,0xffff,2,nullptr));
  FakeSdk last;last.begin(0xfffe);
  assert(bridge::stableMatterEndpoint(last,0xfffe,2,nullptr));assert(last.next==0xffff);
  FakeSdk oom;oom.begin(5);oom.failResume=true;
  assert(!bridge::stableMatterEndpoint(oom,2,0,nullptr));assert(oom.creates==0&&oom.next==5);
  oom.failCreate=true;assert(!bridge::stableMatterEndpoint(oom,5,0,nullptr));
  FakeSdk mismatch;mismatch.begin(1);mismatch.badCounter=true;
  assert(!bridge::stableMatterEndpoint(mismatch,2,0,nullptr));assert(mismatch.endpoints.size()==1);
  std::cout<<"Matter endpoint lifecycle passed: pre-start failure, fresh/restored counters, stable IDs, gaps, 16 lights, exhaustion and allocation failures.\n";
}
