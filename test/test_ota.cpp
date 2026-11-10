#include "ota_format.h"
#include <cassert>
#include <iostream>
using namespace bridge;
int main(){
 uint8_t bytes[64]={},digest[32];uint32_t length=0;
 memcpy(bytes,kOtaMagic,sizeof(kOtaMagic));bytes[25]=2;
 assert(parseOtaHeader(bytes,0x1e0000,length,digest)&&length==512);
 bytes[63]=1;assert(!parseOtaHeader(bytes,0x1e0000,length,digest));bytes[63]=0;
 bytes[20]=1;assert(!parseOtaHeader(bytes,0x1e0000,length,digest));bytes[20]=0;
 assert(!parseOtaHeader(bytes,400,length,digest));bytes[25]=0;assert(!parseOtaHeader(bytes,0x1e0000,length,digest));
 uint8_t image[288]={};image[0]=0xe9;image[32]=0x32;image[33]=0x54;image[34]=0xcd;image[35]=0xab;
 assert(classicEsp32Image(image));image[12]=5;assert(!classicEsp32Image(image));image[12]=0;image[32]=0;assert(!classicEsp32Image(image));
 std::cout<<"OTA header, slot bounds and classic ESP32 application identity passed.\n";
}
