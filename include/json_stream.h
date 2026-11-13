#pragma once
#include "status_api.h"
namespace bridge {
// One serializer type for counting, HTTP and NVS String output. Dispatch only
// happens per buffered block, avoiding multiple copies of ArduinoJson's code.
struct JsonSinkRef {
  void* context;
  size_t (*output)(void*,const uint8_t*,size_t);
  template<class Sink> explicit JsonSinkRef(Sink& sink) : context(&sink),
    output([](void* p,const uint8_t* data,size_t size) { return static_cast<Sink*>(p)->write(data,size); }) {}
  size_t write(const uint8_t* data,size_t size) { return output(context,data,size); }
};
class JsonStreamWriter {
  JsonSinkRef sink;
  StatusWriter<JsonSinkRef> buffer;
public:
  template<class Sink> explicit JsonStreamWriter(Sink& target) : sink(target),buffer(sink) {}
  size_t write(uint8_t value) { return buffer.write(value); }
  size_t write(const uint8_t* data,size_t size) { return buffer.write(data,size); }
  size_t text(const char* value) { return buffer.text(value); }
  bool dropLast(uint8_t value) { return buffer.dropLast(value); }
  bool flush() { return buffer.flush(); }
};
} // namespace bridge
