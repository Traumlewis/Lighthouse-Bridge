#include <cassert>
#include <cstdio>
#include <cstring>
#include "rs_network.h"
#include <unistd.h>
struct Config {
  int static_rx_buf_num,dynamic_rx_buf_num,dynamic_tx_buf_num,tx_buf_type,rx_ba_win;
  unsigned magic,storage,aggregation;void* callbacks;
};
int main() {
  Config original{4,32,32,1,6,0x1234,1,1,nullptr};
  Config bounded=rsNetwork::boundedWifiConfig(original);
  assert(original.dynamic_rx_buf_num==32 && original.dynamic_tx_buf_num==32);
  assert(bounded.dynamic_rx_buf_num==8 && bounded.dynamic_tx_buf_num==4);
  Config expected=original;expected.dynamic_rx_buf_num=8;expected.dynamic_tx_buf_num=4;
  assert(!std::memcmp(&bounded,&expected,sizeof(expected)));
  original.dynamic_rx_buf_num=0;original.static_rx_buf_num=16;original.rx_ba_win=12;
  bounded=rsNetwork::boundedWifiConfig(original);assert(bounded.dynamic_rx_buf_num==16);
  original.static_rx_buf_num=2;original.rx_ba_win=2;
  original.dynamic_rx_buf_num=4;original.dynamic_tx_buf_num=2;
  bounded=rsNetwork::boundedWifiConfig(original);
  assert(bounded.dynamic_rx_buf_num==4 && bounded.dynamic_tx_buf_num==2);
  original.tx_buf_type=0;original.dynamic_tx_buf_num=32;
  assert(rsNetwork::boundedWifiConfig(original).dynamic_tx_buf_num==32);
  for(int error:{EAGAIN,EWOULDBLOCK,ENOMEM,ENOBUFS,EINTR}) assert(rsNetwork::socketResult(-1,error)==-1);
  for(int error:{EBADF,EINVAL,ENETDOWN}) assert(rsNetwork::socketResult(-1,error)==-2);
  assert(rsNetwork::socketResult(1500,ENOMEM)==1500);
  assert(rsNetwork::socketResult(0,EBADF)==0);
  int sockets[2];assert(!socketpair(AF_UNIX,SOCK_DGRAM,0,sockets));
  assert(rsNetwork::peekDatagram(sockets[1])==0);
  const uint8_t packet[]={1,2,3};uint8_t received[3]{};
  assert(send(sockets[0],packet,sizeof(packet),0)==3);
  assert(rsNetwork::peekDatagram(sockets[1])==1);
  assert(rsNetwork::peekDatagram(sockets[1])==1); // must not consume it
  assert(recv(sockets[1],received,sizeof(received),0)==3);
  assert(!std::memcmp(packet,received,3) && rsNetwork::peekDatagram(sockets[1])==0);
  assert(send(sockets[0],packet,0,0)==0);
  assert(rsNetwork::peekDatagram(sockets[1])==1);
  assert(recv(sockets[1],received,sizeof(received),0)==0);
  assert(rsNetwork::peekDatagram(sockets[1])==0);
  close(sockets[0]);close(sockets[1]);assert(rsNetwork::peekDatagram(sockets[1])==-1);
  puts("rs-matter network: bounded public Wi-Fi config, preserved fields/minima and temporary socket errors and real non-consuming datagram readiness passed.");
}
