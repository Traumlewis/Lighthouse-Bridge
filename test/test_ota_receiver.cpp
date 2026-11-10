#include "ota_receiver.h"
#include <vector>
#include <cassert>
#include <iostream>
using namespace bridge;
// Deterministic incremental test digest; firmware IO uses mbedTLS SHA-256.
// Real SHA-256 packaging/corruption is independently covered by Python tests.
struct FakeIO {
 std::vector<uint8_t> data;bool begun=false,aborted=false,failWrite=false;
 bool begin(uint32_t){begun=true;return true;}
 bool write(uint8_t* bytes,size_t count){if(failWrite)return false;data.insert(data.end(),bytes,bytes+count);return true;}
 bool finishHash(uint8_t* digest){std::memset(digest,0,32);for(size_t i=0;i<data.size();++i)digest[i%32]^=data[i];return true;}
 void abort(){aborted=true;}
};
std::vector<uint8_t> file(){
 std::vector<uint8_t> data(1064,0);std::memcpy(data.data(),kOtaMagic,sizeof(kOtaMagic));data[24]=0xe8;data[25]=3;
 auto* image=data.data()+64;image[0]=0xe9;image[32]=0x32;image[33]=0x54;image[34]=0xcd;image[35]=0xab;
 std::memcpy(image+299,kOtaProduct,sizeof(kOtaProduct));
 for(size_t i=0;i<1000;++i)data[28+i%32]^=image[i];
 return data;
}
int main(){
 for(size_t chunk:{1U,2U,19U,63U,64U,65U,287U,288U,351U,352U,353U,512U,2048U}){
  FakeIO io;OtaReceiver<FakeIO> r(io);auto bytes=file();
  for(size_t i=0;i<bytes.size();i+=chunk)assert(!r.feed(bytes.data()+i,std::min(chunk,bytes.size()-i),0x1e0000));
  assert(!r.finish()&&r.ready&&r.started);assert(io.data==std::vector<uint8_t>(bytes.begin()+64,bytes.end()));
  // Validation never selects a boot slot. Explicit abort still works after file-end.
  r.abort();assert(io.aborted&&!r.started&&!r.ready);
 }
 for(unsigned kind=0;kind<6;++kind){
  FakeIO io;OtaReceiver<FakeIO> r(io);auto bytes=file();
  if(kind==0)bytes.back()^=1; // digest
  if(kind==1)bytes.resize(bytes.size()-1); // short payload
  if(kind==2)bytes.push_back(1); // trailing bytes
  if(kind==3)bytes[64+12]=5; // wrong chip before writer start
  if(kind==4)std::memset(bytes.data()+64+299,0,sizeof(kOtaProduct));
  if(kind==5)io.failWrite=true;
  const char* error=r.feed(bytes.data(),bytes.size(),0x1e0000);if(!error)error=r.finish();assert(error&&!r.started&&!r.ready);
  if(kind==3)assert(!io.begun);
 }
 FakeIO io;OtaReceiver<FakeIO> r(io);auto bytes=file();assert(!r.feed(bytes.data(),200,0x1e0000)&&!io.begun);r.abort();r.reset();
 assert(!r.feed(bytes.data(),bytes.size(),0x1e0000)&&!r.finish());r.committed();assert(!r.started&&!r.ready&&!io.aborted);
 std::cout<<"Production OTA receiver passed arbitrary chunking, prefix-before-write, length/digest/product checks, write failures and abort after file end.\n";
}
