#include "tasmota_commands.h"
#include "erased_flash.h"
#include "tasmota_diagnostics.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstring>
using namespace bridge;
int main() {
  TasmotaNetworkPolicy network;
  assert(!network.requestIpv6(0,false,false));
  assert(network.requestIpv6(1,true,false)); // WiFiManager recreated the interface.
  assert(!network.requestIpv6(1000,true,false));
  assert(!network.requestIpv6(30001,true,true));
  assert(network.requestIpv6(30001,true,false));
  assert(!network.requestIpv6(30002,false,false));
  assert(network.requestIpv6(30003,true,true)); // Reconnect reapplies the policy.
  network.lastRequest=UINT32_MAX-100; network.connected=true;
  assert(!network.requestIpv6(50,true,false));
  assert(network.requestIpv6(30000,true,false));
  TasmotaPairingDiagnostics diag;
  diag.record(0,0,0); diag.record(1,0,0x22); diag.record(1,0,0x10);
  assert(diag.received==1 && diag.secureOpcode==0x22);
  diag.record(1,1,5); assert(diag.interactionOpcode==5);
  diag.record(2,0x30,4); diag.record(2,0x3e,6); diag.record(2,0x3e,32);
  assert(diag.generalCommands==16 && diag.credentialCommands==64);
  diag.record(3,0,0); diag.record(4,0,0); diag.record(5,0,0);
  diag.record(6,0,0); diag.record(7,0,0); diag.record(99,0,0);
  assert(diag.mdnsErrors==1 && diag.sendFailures==1 && diag.unacked==1 && diag.completed==1 && diag.errors==1);
  diag.record(8,3,2); assert(diag.windowOpen && diag.advertised && diag.fabrics==2);
  diag.record(8,4,0); assert(diag.windowOpen && diag.advertised && diag.fabrics==2);
  diag.record(8,0,0); assert(!diag.windowOpen && !diag.advertised && diag.fabrics==0);
  diag.received=UINT32_MAX; diag.record(0,0,0); assert(diag.received==UINT32_MAX);

  TasmotaImDiagnostics trace;
  trace.record(9,1,6,0,0x101); trace.record(9,1,6,0,0);
  assert(trace.reads==1 && trace.count==1 && trace.item(0).status==0);
  trace.record(9,0,0x28,0x1234,0x86);
  assert(trace.readErrors==1 && trace.hasError && trace.firstError.id==0x1234);
  trace.record(10,0,0x30,6,0x101); trace.record(10,0,0x30,6,0x81);
  assert(trace.invokes==1 && trace.invokeErrors==1 && trace.lastError.command);
  assert(trace.firstError.id==0x1234 && trace.lastError.id==6);
  trace.record(9,0x10000,0,0,0); trace.record(99,0,0,0,0); trace.record(9,0,0,0,0x102);
  assert(trace.reads==2 && trace.count==3);
  trace.clearPending(); assert(trace.count==0 && trace.readErrors==1 && trace.hasError);
  for(unsigned i=0;i<20;++i) trace.record(9,1,0xfff10001,i,0);
  assert(trace.count==8 && trace.discarded==12 && trace.item(0).id==12 && trace.item(7).id==19);
  assert(trace.item(0).cluster==0xfff10001);
  trace.record(10,0,0x3e,10,0x203);
  assert(trace.removeFabricRequests==1 && trace.invokeErrors==2 && trace.lastError.status==0x203);
  const auto prior=trace.invokes;
  trace.record(9,0,0,0,0x203); trace.record(10,0,0,0,0x300);
  assert(trace.invokes==prior);
  trace.reads=UINT32_MAX;trace.record(9,0,0,0,0);assert(trace.reads==UINT32_MAX);
  trace.record(12,0,0x3f,0,0x88); assert(trace.writes==1 && trace.writeErrors==1 && trace.lastError.write);
  diag.record(1,1,3); diag.record(11,0x86,0); assert(diag.subscriptions==1 && diag.clientErrors==1 && diag.clientStatus==0x86);
  diag.record(11,0,0); assert(diag.clientErrors==1 && diag.clientStatus==0 && diag.clientLastError==0x86);

  assert(matterLabelBytes("short") == 5);
  const std::string ascii(40,'a'); assert(matterLabelBytes(ascii.c_str())==32);
  const std::string cut=std::string(31,'a')+"€"; assert(matterLabelBytes(cut.c_str())==31);
  const std::string exact=std::string(29,'a')+"€"; assert(matterLabelBytes(exact.c_str())==32);
  StationConfig configs[kStations]{};
  StationState states[kStations]{};
  MatterRegistry registry;
  std::string error;
  for (unsigned i=0;i<kStations;++i) {
    configs[i].id=i+1; configs[i].hueId=i+30;
    std::snprintf(configs[i].name,sizeof(configs[i].name),"Test %u",i+1);
    std::snprintf(configs[i].mac,sizeof(configs[i].mac),"FA:00:00:00:00:%02X",i);
  }
  assert(seedMatterRegistry(configs,registry,error));
  for (unsigned i=0;i<kStations;++i) {
    const auto ep=registry.entries[i].endpoint;
    auto accepted=acceptTasmotaCommand(configs,states,registry,ep,1,100,true,false);
    assert(accepted.station==i+1 && accepted.on && states[i].pending);
    assert(states[i].observed==-1 && states[i].observedMs==0);
    const auto generation=states[i].generation;
    acceptTasmotaCommand(configs,states,registry,ep,1,200,true,false);
    assert(states[i].generation==generation && states[i].requestedMs==100);
    accepted=acceptTasmotaCommand(configs,states,registry,ep,0,300,true,false);
    assert(accepted.station==i+1 && !accepted.on && states[i].requested==Target::Sleep);
    assert(states[i].generation!=generation && states[i].observed==-1);
    assert(!acceptTasmotaCommand(configs,states,registry,ep,2,400,true,false).station);
    states[i].observed=11; states[i].observedMs=450; states[i].reachable=true;
    accepted=acceptTasmotaCommand(configs,states,registry,ep,2,500,true,false);
    assert(accepted.station==i+1 && !accepted.on && states[i].observed==11 && states[i].observedMs==450);
    assert(!acceptTasmotaCommand(configs,states,registry,ep,2,60500,true,false).station);
    assert(!acceptTasmotaCommand(configs,states,registry,ep,1,600,false,false).station);
    assert(!acceptTasmotaCommand(configs,states,registry,ep,1,600,true,true).station);
    assert(!acceptTasmotaCommand(configs,states,registry,ep,0x40,600,true,false).station);
    const char old=configs[i].mac[0]; configs[i].mac[0]='B';
    assert(!acceptTasmotaCommand(configs,states,registry,ep,1,600,true,false).station);
    configs[i].mac[0]=old;
  }
  assert(!acceptTasmotaCommand(configs,states,registry,0,1,600,true,false).station);
  assert(!acceptTasmotaCommand(configs,states,registry,2,1,600,true,false).station);
  assert(!acceptTasmotaCommand(configs,states,registry,0xffff,1,600,true,false).station);
  std::vector<uint8_t> flash(0x30000,0xff); size_t readBytes=0;
  auto read=[&](size_t off,uint8_t* dst,size_t size){assert(off+size<=flash.size());std::memcpy(dst,flash.data()+off,size);readBytes+=size;return true;};
  assert(completelyErasedFlash(read,flash.size()) && readBytes==flash.size());
  for (size_t position : {size_t(0),size_t(255),size_t(256),size_t(8191),flash.size()-1}) {
    flash[position]=0; assert(!completelyErasedFlash(read,flash.size())); flash[position]=0xff;
  }
  assert(!completelyErasedFlash(read,0));
  auto failure=[](size_t,uint8_t*,size_t){return false;};
  assert(!completelyErasedFlash(failure,flash.size()));
  assert(completelyErasedFlash(read,257));
  puts("Tasmota policies passed: 16 stable endpoint commands, measured-only state, duplicate coalescing, fresh-only toggle, radio/maintenance rejection, full-partition erased checks.");
}
