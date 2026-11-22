#pragma once

// One transient result, shared by the worker and its private HTTP job.
struct StationInfo {
  char name[32], firmware[80], manufacturer[48], hardware[32], serialText[32];
  uint32_t model, serial;
  uint8_t channel;
  bool hasModel, hasSerial;
};

StationInfo stationInfo{};

static NimBLEAttValue readStationAttribute(NimBLERemoteService* service, const NimBLEUUID& uuid) {
  auto* characteristic = service ? service->getCharacteristic(uuid) : nullptr;
  if (!characteristic || !characteristic->canRead()) return {};
  time_t completed = -2;
  auto value = characteristic->readValue(&completed);
  return completed == -2 ? NimBLEAttValue{} : value;
}

static void stationText(char* output, size_t capacity, const NimBLEAttValue& value) {
  size_t length = 0;
  for (size_t i = 0; i < value.size() && length + 1 < capacity; ++i) {
    const uint8_t byte = value[i];
    if (!byte) break;
    if (byte == '\r') continue;
    output[length++] = byte == '\n' || (byte >= 32 && byte <= 126) ? byte : '?';
  }
  output[length] = 0;
}

static bool stationNumber(const NimBLEAttValue& value, uint32_t& number) {
  if (!value.size() || value.size() > 4) return false;
  number = 0;
  // Preserve the existing big-endian decoding for binary values.
  for (size_t i = 0; i < value.size(); ++i) number = (number << 8) | value[i];
  return true;
}

static void readStationSerial(NimBLERemoteService* device) {
  const auto value = readStationAttribute(device, NimBLEUUID(uint16_t(0x2a25)));
  size_t length = value.size();
  while (length && value[length - 1] == 0) --length;
  bool text = length && length < sizeof stationInfo.serialText;
  for (size_t i = 0; text && i < length; ++i) text = value[i] >= 32 && value[i] <= 126;
  if (text) {
    memcpy(stationInfo.serialText, value.data(), length);
  } else {
    stationInfo.hasSerial = stationNumber(value, stationInfo.serial);
  }
}

static bool readStationInfo(NimBLEClient* client, NimBLERemoteService* control) {
  stationInfo = {};
  if (!control) return false;
  uint32_t channel;
  if (stationNumber(readStationAttribute(control, NimBLEUUID("00001524-1212-efde-1523-785feabcd124")), channel) &&
      channel >= 1 && channel <= 16) stationInfo.channel = channel;
  stationText(stationInfo.name, sizeof stationInfo.name,
              readStationAttribute(client->getService(NimBLEUUID(uint16_t(0x1800))), NimBLEUUID(uint16_t(0x2a00))));
  auto* device = client->getService(NimBLEUUID(uint16_t(0x180a)));
  if (device) {
    stationText(stationInfo.firmware, sizeof stationInfo.firmware, readStationAttribute(device, NimBLEUUID(uint16_t(0x2a26))));
    stationText(stationInfo.manufacturer, sizeof stationInfo.manufacturer, readStationAttribute(device, NimBLEUUID(uint16_t(0x2a29))));
    stationText(stationInfo.hardware, sizeof stationInfo.hardware, readStationAttribute(device, NimBLEUUID(uint16_t(0x2a27))));
    stationInfo.hasModel = stationNumber(readStationAttribute(device, NimBLEUUID(uint16_t(0x2a24))), stationInfo.model);
    readStationSerial(device);
  }
  return true;
}

static void writeStationInfo(JsonObject out) {
  out["type"] = "Lighthouse V2";
  if (stationInfo.name[0]) out["name"] = stationInfo.name;
  if (stationInfo.firmware[0]) out["firmware"] = stationInfo.firmware;
  if (stationInfo.channel) out["channel"] = stationInfo.channel;
  if (stationInfo.manufacturer[0]) out["manufacturer"] = stationInfo.manufacturer;
  if (stationInfo.hasModel) out["model"] = stationInfo.model;
  if (stationInfo.serialText[0]) out["serial"] = stationInfo.serialText;
  else if (stationInfo.hasSerial) out["serial"] = stationInfo.serial;
  if (stationInfo.hardware[0]) out["hardware"] = stationInfo.hardware;
}
