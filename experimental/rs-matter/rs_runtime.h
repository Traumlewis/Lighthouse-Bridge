#pragma once
#include <stddef.h>
#include <stdint.h>
struct RsStation { uint16_t endpoint; char name[41]; char unique[40]; };
struct RsConfig { uint32_t passcode; uint16_t discriminator, count; RsStation stations[16]; char serial[33]; };
extern "C" {
bool lh_rs_begin(const RsConfig* configuration);
int lh_rs_pump();
bool lh_rs_open();
bool lh_rs_pending();
size_t lh_rs_stack_bytes();
size_t lh_rs_task_bytes();
bool lh_rs_info(char* code,size_t capacity,bool* paired,bool* open);
bool lh_queue_power(uint16_t endpoint,unsigned command);
uint32_t lh_rs_observed(uint16_t endpoint);
}
