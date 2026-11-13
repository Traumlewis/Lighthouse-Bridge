#pragma once
#include <stddef.h>
#include <stdint.h>
extern "C" {
bool lh_tasmota_begin(const char* configuration);
bool lh_tasmota_report(uint16_t endpoint, bool on, bool reachable);
bool lh_tasmota_info(char* json, size_t size);
bool lh_tasmota_commission();
bool lh_tasmota_checkpoint();
bool lh_tasmota_fs_ready();
size_t lh_tasmota_vm_heap();
void lh_tasmota_pump();
bool lh_tasmota_mount_fs();
bool lh_queue_power(uint16_t endpoint, unsigned command);
void lighthouseSetup();
void lighthouseLoop();
}
