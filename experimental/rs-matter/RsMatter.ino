#define LH_MATTER_BUILD 1
#define LH_RS_MATTER_BUILD 1
#define LH_FIRMWARE_VERSION "0.1.2-rs-matter"
#include <Arduino.h>
// Owner 0.1.1 measured <=5964 bytes used of 24576 during startup/idle.
// Keep >10 KiB beyond that measured use; pairing/OTA peaks still need testing.
SET_LOOP_TASK_STACK_SIZE(16384);
#include "main.cpp"
