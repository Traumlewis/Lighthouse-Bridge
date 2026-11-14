#define LH_MATTER_BUILD 1
#define LH_RS_MATTER_BUILD 1
#define LH_FIRMWARE_VERSION "0.1.1-rs-matter"
#include <Arduino.h>
// rs-matter documents about 20 KiB task stack in its smallest configuration.
SET_LOOP_TASK_STACK_SIZE(24576);
#include "main.cpp"
