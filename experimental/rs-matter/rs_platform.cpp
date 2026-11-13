// ESP-IDF transport and NVS only. No Matter C++/CHIP stack is linked.
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <fcntl.h>
#include <errno.h>
#include <nvs.h>
#include "rs_runtime.h"
struct NetInfo { uint8_t mac[8],ip4[4],ip6[4][16]; uint32_t ipv6_count, interface; bool connected; };
static_assert(sizeof(NetInfo)==88,"Rust interface ABI");
struct WireAddr { uint8_t ip[16]; uint16_t port; uint32_t scope; };
static_assert(offsetof(WireAddr,scope)==20 && sizeof(WireAddr)==24,"Rust socket ABI");
static_assert(sizeof(RsStation)==84 && offsetof(RsConfig,serial)==1352,"Rust configuration ABI");
static portMUX_TYPE rsCritical = portMUX_INITIALIZER_UNLOCKED;
extern "C" uint32_t lh_rs_lock() { portENTER_CRITICAL(&rsCritical); return 0; }
extern "C" void lh_rs_unlock(uint32_t) { portEXIT_CRITICAL(&rsCritical); }
extern "C" uint64_t lh_rs_time() { return esp_timer_get_time(); }
extern "C" void lh_rs_random(uint8_t* out,size_t size) { esp_fill_random(out,size); }
extern "C" void lh_rs_fault() { Serial.println("RS_MATTER_PANIC"); abort(); }
extern "C" int lh_rs_store(unsigned op,uint16_t id,uint8_t* bytes,size_t size) {
  nvs_handle_t handle; if(nvs_open("lhrsmatter",NVS_READWRITE,&handle)!=ESP_OK) return -2;
  char key[8]; snprintf(key,sizeof(key),"b%04x",id); int result=-2;
  if(op==0) {
    size_t actual=size; esp_err_t error=nvs_get_blob(handle,key,bytes,&actual);
    result=error==ESP_ERR_NVS_NOT_FOUND ? -1 : error==ESP_OK && actual<=size ? static_cast<int>(actual) : -2;
  } else if(op==1) {
    if(nvs_set_blob(handle,key,bytes,size)==ESP_OK && nvs_commit(handle)==ESP_OK) result=0;
  } else if(op==2) {
    esp_err_t error=nvs_erase_key(handle,key);
    if((error==ESP_OK || error==ESP_ERR_NVS_NOT_FOUND) && nvs_commit(handle)==ESP_OK) result=0;
  }
  nvs_close(handle); return result;
}
extern "C" bool lh_rs_net(NetInfo* out) {
  *out={}; esp_netif_t* net=esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"); if(!net) return false;
  out->connected=WiFi.status()==WL_CONNECTED;
  esp_netif_get_mac(net,out->mac); out->interface=esp_netif_get_netif_impl_index(net);
  esp_netif_ip_info_t ip{};
  if(esp_netif_get_ip_info(net,&ip)==ESP_OK) memcpy(out->ip4,&ip.ip.addr,4);
  esp_ip6_addr_t addresses[CONFIG_LWIP_IPV6_NUM_ADDRESSES]{};
  int count=esp_netif_get_all_ip6(net,addresses);
  out->ipv6_count=std::max(0,std::min(count,4));
  static bool wasConnected=false;static uint32_t attempted=0;
  if(out->connected && (!wasConnected || (!out->ipv6_count && millis()-attempted>=30000))){
    attempted=millis();
    if(!WiFi.enableIPv6()) Serial.println("rs-matter IPv6 request failed.");
    const esp_err_t policy=esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    if(policy!=ESP_OK) Serial.printf("rs-matter Wi-Fi modem sleep failed: code=%d\n",policy);
  }
  wasConnected=out->connected;
  for(unsigned i=0;i<out->ipv6_count;++i) memcpy(out->ip6[i],addresses[i].addr,16);
  return true;
}
extern "C" int lh_rs_socket(uint16_t port,const uint8_t* ip4,uint32_t interface) {
  int fd=socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP); if(fd<0) return -1;
  int yes=1,no=0; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
  setsockopt(fd,IPPROTO_IPV6,IPV6_V6ONLY,&no,sizeof(no));
  setsockopt(fd,IPPROTO_IPV6,IPV6_MULTICAST_IF,&interface,sizeof(interface));
  in_addr ip{};memcpy(&ip,ip4,4);setsockopt(fd,IPPROTO_IP,IP_MULTICAST_IF,&ip,sizeof(ip));
  sockaddr_in6 addr{};addr.sin6_family=AF_INET6;addr.sin6_port=htons(port);addr.sin6_addr=in6addr_any;
  if(bind(fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))<0 || fcntl(fd,F_SETFL,O_NONBLOCK)<0) {close(fd);return -1;}
  return fd;
}
extern "C" void lh_rs_close(int fd) { close(fd); }
extern "C" int lh_rs_recv(int fd,uint8_t* data,size_t size,WireAddr* remote) {
  sockaddr_in6 address{}; socklen_t length=sizeof(address);
  int n=recvfrom(fd,data,size,MSG_DONTWAIT,reinterpret_cast<sockaddr*>(&address),&length);
  if(n<0) return errno==EAGAIN || errno==EWOULDBLOCK ? -1 : -2;
  if(address.sin6_family!=AF_INET6) return -2;
  memcpy(remote->ip,&address.sin6_addr,16);remote->port=ntohs(address.sin6_port);remote->scope=address.sin6_scope_id;
  return n;
}
extern "C" int lh_rs_send(int fd,const uint8_t* data,size_t size,const WireAddr* remote) {
  sockaddr_in6 address{};address.sin6_family=AF_INET6;address.sin6_port=htons(remote->port);
  memcpy(&address.sin6_addr,remote->ip,16);address.sin6_scope_id=remote->scope;
  return sendto(fd,data,size,MSG_DONTWAIT,reinterpret_cast<sockaddr*>(&address),sizeof(address));
}
extern "C" bool lh_rs_multicast(int fd,bool join,const uint8_t* group,uint32_t interface) {
  if(group[10]==0xff && group[11]==0xff) {
    ip_mreq req{};memcpy(&req.imr_multiaddr,group+12,4);req.imr_interface.s_addr=WiFi.localIP();
    return setsockopt(fd,IPPROTO_IP,join?IP_ADD_MEMBERSHIP:IP_DROP_MEMBERSHIP,&req,sizeof(req))==0;
  }
  ipv6_mreq req{};memcpy(&req.ipv6mr_multiaddr,group,16);req.ipv6mr_interface=interface;
  return setsockopt(fd,IPPROTO_IPV6,join?IPV6_JOIN_GROUP:IPV6_LEAVE_GROUP,&req,sizeof(req))==0;
}

extern "C" void* lh_rs_alloc(size_t size,size_t align) {return heap_caps_aligned_alloc(std::max(align,sizeof(void*)),size,MALLOC_CAP_8BIT);}
extern "C" uint32_t lh_rs_wifi(uint8_t* ssid,uint8_t* password) {
  wifi_config_t saved{};
  if(esp_wifi_get_config(WIFI_IF_STA,&saved)!=ESP_OK) return 0;
  memcpy(ssid,saved.sta.ssid,32);memcpy(password,saved.sta.password,64);
  return static_cast<uint32_t>(strnlen(reinterpret_cast<char*>(saved.sta.ssid),32)) |
    (static_cast<uint32_t>(strnlen(reinterpret_cast<char*>(saved.sta.password),64))<<16);
}
