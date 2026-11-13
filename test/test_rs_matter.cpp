#include "rs_matter_commands.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace bridge;
int main() {
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
    auto accepted=acceptRsMatterCommand(configs,states,registry,ep,1,100,true,false);
    assert(accepted.station==i+1 && accepted.on && states[i].pending);
    assert(states[i].observed==-1 && states[i].observedMs==0);
    const auto generation=states[i].generation;
    acceptRsMatterCommand(configs,states,registry,ep,1,200,true,false);
    assert(states[i].generation==generation && states[i].requestedMs==100);
    accepted=acceptRsMatterCommand(configs,states,registry,ep,0,300,true,false);
    assert(accepted.station==i+1 && !accepted.on && states[i].requested==Target::Sleep);
    assert(states[i].generation!=generation && states[i].observed==-1);
    assert(!acceptRsMatterCommand(configs,states,registry,ep,2,400,true,false).station);
    states[i].observed=11; states[i].observedMs=450; states[i].reachable=true;
    accepted=acceptRsMatterCommand(configs,states,registry,ep,2,500,true,false);
    assert(accepted.station==i+1 && !accepted.on && states[i].observed==11 && states[i].observedMs==450);
    assert(!acceptRsMatterCommand(configs,states,registry,ep,2,60500,true,false).station);
    assert(!acceptRsMatterCommand(configs,states,registry,ep,1,600,false,false).station);
    assert(!acceptRsMatterCommand(configs,states,registry,ep,1,600,true,true).station);
    assert(!acceptRsMatterCommand(configs,states,registry,ep,0x40,600,true,false).station);
    const char old=configs[i].mac[0]; configs[i].mac[0]='B';
    assert(!acceptRsMatterCommand(configs,states,registry,ep,1,600,true,false).station);
    configs[i].mac[0]=old;
  }
  assert(!acceptRsMatterCommand(configs,states,registry,0,1,600,true,false).station);
  assert(!acceptRsMatterCommand(configs,states,registry,2,1,600,true,false).station);
  assert(!acceptRsMatterCommand(configs,states,registry,0xffff,1,600,true,false).station);
  puts("rs-matter: 16 endpoint commands, measured state, coalescing, fresh toggle, maintenance gates passed.");
}
