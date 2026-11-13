#pragma once
struct ConfigStringSink {
  String& value;
  size_t write(const uint8_t* data,size_t size) {
    const size_t before = value.length();
    value.concat(reinterpret_cast<const char*>(data),size);
    return value.length()-before;
  }
};
template<class Render> bool makeConfigString(String& value,Render render) {
  CountSink count; JsonStreamWriter counter(count);
  if (!render(counter) || !value.reserve(count.size)) return false;
  ConfigStringSink sink{value}; JsonStreamWriter writer(sink);
  return render(writer) && value.length() == count.size;
}
