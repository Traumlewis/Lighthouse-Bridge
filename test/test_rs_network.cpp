#include <cassert>
#include <cstdio>
#include <cstring>
#include "rs_network.h"
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
  puts("rs-matter network: bounded public Wi-Fi config, preserved fields/minima and temporary socket error classification passed.");
}
