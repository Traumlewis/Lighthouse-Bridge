#pragma once
#include <string>
#include <stdint.h>
#include <unistd.h>
using String = std::string;
using PGM_P = const char*;
uint32_t millis();
void delay(unsigned);
class NetworkClient {
public:
  int descriptor = -1;
  int fd() const { return descriptor; }
  void stop() { if (descriptor >= 0) close(descriptor); descriptor = -1; }
};
class WebServer {
protected:
  struct RequestArgument { String key,value; };
  RequestArgument* _currentArgs = nullptr;
  int _currentArgCount = 0;
  NetworkClient _currentClient;
  virtual size_t _currentClientWrite(const char*,size_t) { return 0; }
  virtual size_t _currentClientWrite_P(PGM_P,size_t) { return 0; }
public:
  explicit WebServer(uint16_t) {}
  void handleClient() {}
};
