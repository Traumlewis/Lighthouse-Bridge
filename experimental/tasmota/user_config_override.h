// Lighthouse Tasmota prototype. Upstream defaults are pruned by lighthouse_features.h.
#pragma once
#define FIRMWARE_LITE
#define USE_MATTER_DEVICE
#undef SERIAL_LOG_LEVEL
#define SERIAL_LOG_LEVEL LOG_LEVEL_NONE
#undef WEB_LOG_LEVEL
#define WEB_LOG_LEVEL LOG_LEVEL_NONE
#undef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL LOG_LEVEL_NONE
#undef MQTT_HOST
#define MQTT_HOST ""
#undef MODULE
#define MODULE USER_MODULE
#undef USER_TEMPLATE
#define USER_TEMPLATE "{\"NAME\":\"Lighthouse\",\"GPIO\":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],\"FLAG\":0,\"BASE\":1}"
