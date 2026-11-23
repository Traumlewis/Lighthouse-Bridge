#pragma once
#include "sdkconfig.h"
// Apply the same host configuration after the target SDK defaults in every TU.
// C3/S3 SDK defaults otherwise overwrite command-line -D values.
#undef CONFIG_BT_NIMBLE_MAX_CONNECTIONS
#undef CONFIG_NIMBLE_MAX_CONNECTIONS
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 1
#define CONFIG_NIMBLE_MAX_CONNECTIONS 1
#undef CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#undef CONFIG_NIMBLE_ROLE_PERIPHERAL
#undef CONFIG_BT_NIMBLE_ROLE_BROADCASTER
#undef CONFIG_NIMBLE_ROLE_BROADCASTER
#define CONFIG_BT_NIMBLE_ROLE_PERIPHERAL 0
#define CONFIG_BT_NIMBLE_ROLE_BROADCASTER 0

#if defined(CONFIG_IDF_TARGET_ESP32C3)
// A single Lighthouse GATT client does not need the SDK's multi-link pool sizes.
// Leave block sizes, controller settings and advertising event pools intact.
#undef CONFIG_BT_NIMBLE_MSYS_2_BLOCK_COUNT
#define CONFIG_BT_NIMBLE_MSYS_2_BLOCK_COUNT 12
#undef CONFIG_BT_NIMBLE_TRANSPORT_ACL_FROM_LL_COUNT
#define CONFIG_BT_NIMBLE_TRANSPORT_ACL_FROM_LL_COUNT 12
#endif
