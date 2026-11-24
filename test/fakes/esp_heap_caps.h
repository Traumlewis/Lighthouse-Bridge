#pragma once
#include <stddef.h>
#include <stdint.h>

constexpr uint32_t MALLOC_CAP_INTERNAL = 1;
constexpr uint32_t MALLOC_CAP_DMA = 2;
size_t heap_caps_get_largest_free_block(uint32_t caps);
