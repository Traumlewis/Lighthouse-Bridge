#include "startup_sequence.h"
#include <cassert>
#include <string>
#include <iostream>
struct Services {
  bool reserveOk,bleOk,matterOk,web=false,available=false,reserved=false,bleStarted=false,matterStarted=false;
  unsigned reads=0,writes=0;
  std::string trace,error;
  Services(bool reserve,bool ble,bool matter):reserveOk(reserve),bleOk(ble),matterOk(matter){}
  void tick(){if(available)++reads;assert(writes==0);}
  bool reserveWorker(){trace+='R';reserved=reserveOk;tick();return reserved;}
  void startWeb(){trace+='W';web=true;tick();}
  bool startMatter(){trace+='M';assert(web&&reserved&&bleStarted&&!available);matterStarted=matterOk;tick();return matterOk;}
  bool startBle(){trace+='B';assert(web&&!available);bleStarted=bleOk;tick();return bleOk;}
  void failed(const char* message){trace+='F';assert(web&&!available);error=message;tick();}
  void releaseWorker(){trace+='E';assert(web&&reserved&&bleStarted);available=true;tick();}
};
int main(){
  assert(!bridge::startupHttpDue(2999,0) && bridge::startupHttpDue(3000,0));
  assert(!bridge::startupHttpDue(100,0xfffffff0U) && bridge::startupHttpDue(4000,0xfffffff0U));
  Services success(true,true,true);bridge::startMatterServices(success);
  assert(success.trace=="RWBME"&&success.web&&success.available&&success.reads==1&&success.writes==0);
  Services noWorker(false,true,true);bridge::startMatterServices(noWorker);
  assert(noWorker.trace=="RWF"&&noWorker.web&&!noWorker.available&&noWorker.reads==0);
  assert(noWorker.error=="BLE_WORKER_FAILED");
  Services noRadio(true,false,true);bridge::startMatterServices(noRadio);
  assert(noRadio.trace=="RWBF"&&noRadio.web&&!noRadio.available&&noRadio.reads==0);
  assert(noRadio.error=="BLE_INIT_FAILED");
  Services noMatter(true,true,false);bridge::startMatterServices(noMatter);
  assert(noMatter.trace=="RWBMF"&&noMatter.web&&!noMatter.available&&!noMatter.matterStarted&&noMatter.reads==0&&noMatter.writes==0);
  assert(noMatter.error=="MATTER_START_FAILED");
  std::cout<<"Startup orchestration passed: reserve before Matter, gate radio work, preserve HTTP on worker/radio failure, no automatic writes.\n";
}
