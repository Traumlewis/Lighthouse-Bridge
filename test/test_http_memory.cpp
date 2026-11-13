#include "WebServer.h"
#include "matter_http.h"
#include "config_stream.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <cstdio>
using namespace bridge;
uint32_t clockMs = 0;
int peer = -1;
bool drain = true;
unsigned pausesBeforeDrain = 20;
char received[40000]; size_t receivedSize = 0;
uint32_t millis() { return clockMs; }
void delay(unsigned ms) {
  clockMs += ms;
  if (pausesBeforeDrain) { --pausesBeforeDrain; return; }
  if (!drain || peer < 0) return;
  int count;
  while ((count = recv(peer,received+receivedSize,sizeof(received)-receivedSize,MSG_DONTWAIT)) > 0)
    receivedSize += size_t(count);
}
struct Server : MatterWebServer {
  RequestArgument args[1];
  Server() : MatterWebServer(80) { _currentArgs = args; _currentArgCount = 1; }
  void socket(int descriptor) { _currentClient.descriptor = descriptor; }
  int socket() { return _currentClient.fd(); }
  size_t header(const char* text) { return _currentClientWrite(text,std::strlen(text)); }
};
struct Sink {
  std::string data; bool fail = false;
  size_t write(const uint8_t* bytes,size_t size) {
    if (fail) return 0;
    data.append(reinterpret_cast<const char*>(bytes),size); return size;
  }
};
void transportChecks() {
  struct SlowTransport {
    uint32_t time = 0;
    uint32_t now() { return time; }
    void pause() { time += 100; }
    int tryWrite(const uint8_t*,size_t) { return 1; }
  } slow;
  uint8_t slowPayload[200]{};
  assert(writeSocketBody(slow,slowPayload,sizeof(slowPayload),0) == 80 && slow.time == 8000);
  Server server;
  server.args[0].key = "plain"; server.args[0].value.assign(4096,'x');
  const char* owned = server.args[0].value.data();
  const String body = server.takePlainBody();
  assert(body.size() == 4096 && body.data() == owned && server.args[0].value.empty());
  int sockets[2]; assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));
  server.socket(sockets[0]); peer = sockets[1];
  int small = 512; setsockopt(sockets[0],SOL_SOCKET,SO_SNDBUF,&small,sizeof(small));
  clockMs = UINT32_MAX-100; // The deadline also survives millis wrap.
  server.handleClient();
  std::vector<uint8_t> payload(16908);
  for (size_t i = 0; i < payload.size(); ++i) payload[i] = uint8_t(i);
  assert(server.header("HTTP/1.1 200 OK\r\n\r\n") == 19);
  assert(server.writeBytes(payload.data(),payload.size()) == payload.size());
  delay(1);
  assert(receivedSize == payload.size()+19);
  assert(pausesBeforeDrain == 0);
  assert(!std::memcmp(received+19,payload.data(),payload.size()));
  close(peer); peer = -1; server.socket(-1); close(sockets[0]);
  assert(!socketpair(AF_UNIX,SOCK_STREAM,0,sockets));
  server.socket(sockets[0]); peer = sockets[1]; drain = false;
  setsockopt(sockets[0],SOL_SOCKET,SO_SNDBUF,&small,sizeof(small));
  server.handleClient(); const uint32_t start = clockMs;
  assert(server.writeBytes(payload.data(),payload.size()) < payload.size());
  assert(clockMs-start >= 2500 && clockMs-start <= 8000 && server.socket() < 0);
  close(peer); peer = -1;
}
void configChecks() {
  StationConfig configs[kStations];
  for (unsigned i = 0; i < kStations; ++i) {
    configs[i].id = i+1; configs[i].hueId = i+6; configs[i].alexaCompat = i%2;
    std::memset(configs[i].name,'a'+i,40);
    std::snprintf(configs[i].mac,18,"AA:BB:CC:DD:EE:%02X",i);
  }
  DynamicJsonDocument request(kStationJsonCapacity);
  writeStationList(request.createNestedArray("stations"),configs);
  std::string input; serializeJson(request,input);
  std::vector<char> body(input.begin(),input.end()); body.push_back(0);
  StaticJsonDocument<kConfigRequestCapacity> parsed;
  assert(!deserializeJson(parsed,body.data()));
  StationConfig next[kStations]; uint32_t nextId = 30; std::string error;
  assert(parseStationList(parsed["stations"],next,configs,nextId,false,false,error));
  body.clear(); parsed.clear(); // No parser/request string may be borrowed now.
  Sink persisted; JsonStreamWriter writer(persisted);
  assert(streamConfiguration(writer,next,nextId));
  DynamicJsonDocument restored(kStationJsonCapacity);
  assert(!deserializeJson(restored,persisted.data));
  assert(restored["stations"].size() == 16 && restored["stations"][15]["hue_id"] == 21);
  assert(restored["stations"][15]["name"].as<std::string>().size() == 40);
  CountSink counter; JsonStreamWriter countWriter(counter);
  assert(streamConfiguration(countWriter,next,nextId) && counter.size == persisted.data.size());
  Sink reply; JsonStreamWriter replyWriter(reply);
  assert(streamConfiguration(replyWriter,next,nextId,true));
  restored.clear(); assert(!deserializeJson(restored,reply.data));
  assert(restored["saved"] == true && restored["restarting"] == true);
  assert(!restored["stations"][0].containsKey("hue_id"));
  MatterRegistry registry; registry.count = 32; registry.nextEndpoint = 100;
  for (unsigned i = 0; i < 32; ++i) {
    registry.entries[i].hueId = i+6; registry.entries[i].endpoint = i+3;
    std::snprintf(registry.entries[i].mac,18,"AA:BB:CC:DD:EE:%02X",i);
  }
  Sink checkpoint; JsonStreamWriter checkpointWriter(checkpoint);
  assert(streamRegistry(checkpointWriter,registry));
  DynamicJsonDocument regDoc(kMatterRegistryJsonCapacity);
  assert(!deserializeJson(regDoc,checkpoint.data));
  MatterRegistry recovered;
  assert(parseMatterRegistry(regDoc.as<JsonVariantConst>(),recovered,error) && recovered.count == 32);
  Sink failed; failed.fail = true; JsonStreamWriter failedWriter(failed);
  assert(!streamConfiguration(failedWriter,next,nextId));
}
int main() {
  transportChecks(); configChecks();
  puts("Matter HTTP/config memory tests passed: socket pressure/deadlines/wrap, moved request, zero-copy 16 stations, 32 bindings and reboot reply.");
}
