#pragma once
#include <cstring>
namespace bridge {
enum class NetworkMode { Home, Local };
inline NetworkMode storedNetworkMode(const char* value) {
  return value && !std::strcmp(value,"local") ? NetworkMode::Local : NetworkMode::Home;
}
inline const char* networkModeName(NetworkMode mode) { return mode == NetworkMode::Local ? "local" : "home"; }
// One atomic NVS string; never rewrite Wi-Fi credentials, authentication or fabrics.
// The caller changes/restarts the running network only after this succeeds.
template<class Store> bool saveNetworkMode(Store& store,NetworkMode mode) {
  const char* value = networkModeName(mode);
  return store.putString("wifi_mode",value) == std::strlen(value);
}
// A temporary setup reboot never changes the saved home/local choice.
template<class Store> bool requestWifiSetup(Store& store) {
  return store.putBool("wifi_setup",true) == 1;
}
template<class Store> bool consumeWifiSetup(Store& store,bool& requested) {
  requested = false;
  if (!store.getBool("wifi_setup",false)) return true;
  if (store.putBool("wifi_setup",false) != 1) return false;
  requested = true;
  return true;
}
}
