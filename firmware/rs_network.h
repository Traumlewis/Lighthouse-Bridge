#pragma once
#include <algorithm>
#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>
namespace rsNetwork {
constexpr int kDynamicTxBuffers=4;
constexpr int kDynamicRxBuffers=8;
// Apply after Arduino's defaults, using only the public SDK config. Preserve
// static buffers, aggregation, storage, callbacks and all unrelated fields.
template<class Config> Config boundedWifiConfig(const Config& original) {
  Config cfg=original;
  const int rxMinimum=std::max(kDynamicRxBuffers,std::max(cfg.static_rx_buf_num,cfg.rx_ba_win));
  if (!cfg.dynamic_rx_buf_num || cfg.dynamic_rx_buf_num>rxMinimum) cfg.dynamic_rx_buf_num=rxMinimum;
  if (cfg.tx_buf_type==1 && cfg.dynamic_tx_buf_num>kDynamicTxBuffers) cfg.dynamic_tx_buf_num=kDynamicTxBuffers;
  return cfg;
}
inline bool temporarySocketError(int error) {
  return error==EAGAIN || error==EWOULDBLOCK || error==ENOMEM || error==ENOBUFS || error==EINTR;
}
inline int socketResult(int result,int error) {
  return result>=0 ? result : temporarySocketError(error) ? -1 : -2;
}
// Probe without consuming a datagram or reserving the Matter shared RX buffer.
// A zero-length UDP datagram is readable too; this is not TCP disconnect.
inline int peekDatagram(int fd) {
  uint8_t byte;
  const int n=::recv(fd,&byte,1,MSG_PEEK|MSG_DONTWAIT);
  return n>=0 ? 1 : temporarySocketError(errno) ? 0 : -1;
}
}
