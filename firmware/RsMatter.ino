#define LH_FIRMWARE_VERSION "0.1.38"
#include <Arduino.h>
// Owner 0.1.7 commissioning left only 692 bytes of the 16-KiB task stack.
// Move 2 KiB here from the compile-checked Rust future arena; their sum stays 32 KiB.
SET_LOOP_TASK_STACK_SIZE(18432);
#include "main.cpp"
