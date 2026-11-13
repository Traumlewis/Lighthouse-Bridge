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
struct CleanupSdk {
  struct Cluster { Cluster* next; bool alive; };
  struct Endpoint { bool initialized; Cluster* first; bool removed; };
  using Handle = Endpoint*;
  unsigned metadataDeletes = 0, shutdowns = 0;
  bool unsafeShutdown = false, failCluster = false, failEndpoint = false;
  Cluster* firstCluster(Handle endpoint) { return endpoint->first; }
  Cluster* nextCluster(Cluster* cluster) { assert(cluster->alive); return cluster->next; }
  Endpoint* owner = nullptr;
  bool destroyCluster(Cluster* cluster) {
    if (failCluster) return false;
    assert(owner->first == cluster && cluster->alive);
    owner->first = cluster->next; cluster->alive = false; ++metadataDeletes; return true;
  }
  bool destroyEndpoint(Handle endpoint) {
    if (failEndpoint) return false;
    for (auto* cluster = endpoint->first; cluster; cluster = cluster->next) {
      if (!endpoint->initialized) { unsafeShutdown = true; return false; }
      ++shutdowns;
    }
    endpoint->removed = true; return true;
  }
};
int main(){
  for (bool initialized : {false,true}) {
    CleanupSdk sdk;
    CleanupSdk::Cluster second{nullptr,true}, first{&second,true};
    CleanupSdk::Endpoint endpoint{initialized,&first,false}; sdk.owner = &endpoint;
    if (!initialized) {
      // Pinned SDK endpoint::destroy invokes Descriptor shutdown before init:
      // LazyRegisteredServerCluster::Destroy would abort on !IsConstructed().
      assert(!sdk.destroyEndpoint(&endpoint) && sdk.unsafeShutdown);
      sdk.unsafeShutdown = false;
    }
    assert(bridge::discardMatterEndpoint(sdk,&endpoint,initialized));
    assert(endpoint.removed && !sdk.unsafeShutdown);
    assert(sdk.shutdowns == (initialized ? 2U : 0U));
    assert(sdk.metadataDeletes == (initialized ? 0U : 2U));
  }
  CleanupSdk broken; CleanupSdk::Cluster cluster{nullptr,true};
  CleanupSdk::Endpoint partial{false,&cluster,false}; broken.owner=&partial; broken.failCluster=true;
  assert(!bridge::discardMatterEndpoint(broken,&partial,false));
  assert(!partial.removed && !broken.unsafeShutdown && cluster.alive);
  broken.failCluster=false; broken.failEndpoint=true;
  assert(!bridge::discardMatterEndpoint(broken,&partial,false));
  assert(!partial.removed && broken.metadataDeletes==1 && !broken.unsafeShutdown);
  broken.failEndpoint=false;
  assert(bridge::discardMatterEndpoint(broken,&partial,false));
  bridge::MatterLightBudget budget;
  assert(!budget.allows(6143) && budget.allows(6144));
  budget.record(14000,3200);
  assert(budget.cost==10800 && !budget.allows(3200));
  assert(!budget.allows(12847) && budget.allows(12848));
  budget.record(3000,4000); assert(budget.cost==10800);
  assert(!budget.allows(0) && budget.allows(0xffffffff));
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
  std::cout<<"Matter endpoint lifecycle passed: pre-start failure, fresh/restored counters, stable IDs, gaps, 16 lights, exhaustion, allocation failures, uninitialized shutdown regression and light memory admission.\n";
}
