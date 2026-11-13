#pragma once
#include "socket_write.h"
#include <errno.h>
#include <sys/socket.h>
#ifdef LH_RS_MATTER_BUILD
extern "C" int lh_rs_pump();
#endif

class MatterWebServer : public WebServer {
  uint32_t writeStarted = 0;
  bool writing = false;
  struct Transport {
    NetworkClient& client;
    uint32_t now() { return millis(); }
    void pause() {
#ifdef LH_RS_MATTER_BUILD
      lh_rs_pump();
#endif
      delay(1);
    }
    int tryWrite(const uint8_t* data,size_t size) {
      if (client.fd() < 0) return 0;
      const int count = ::send(client.fd(),data,size,MSG_DONTWAIT);
      if (count >= 0) return count;
      return errno == EAGAIN || errno == ENOMEM || errno == ENOBUFS || errno == EINTR ? -1 : -2;
    }
  };
protected:
  size_t _currentClientWrite(const char* bytes,size_t size) override {
    return writeBytes(reinterpret_cast<const uint8_t*>(bytes),size);
  }
  size_t _currentClientWrite_P(PGM_P bytes,size_t size) override { return _currentClientWrite(bytes,size); }
public:
  explicit MatterWebServer(uint16_t port) : WebServer(port) {}
  void handleClient() { writing = false; WebServer::handleClient(); }
  size_t writeBytes(const uint8_t* bytes,size_t size) {
#ifdef LH_RS_MATTER_BUILD
    lh_rs_pump();
#endif
    if (!writing) { writing = true; writeStarted = millis(); }
    Transport transport{_currentClient};
    const size_t sent = bridge::writeSocketBody(transport,bytes,size,writeStarted);
    if (sent != size) _currentClient.stop();
    return sent;
  }
  // Use the pinned WebServer's protected request arguments without copying the
  // full JSON body. The returned String owns it until zero-copy parsing ends.
  String takePlainBody() {
    for (int i = 0; i < _currentArgCount; ++i)
      if (_currentArgs[i].key == "plain") return std::move(_currentArgs[i].value);
    return String();
  }
};
