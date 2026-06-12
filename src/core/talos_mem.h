#pragma once

#include "vx_cpu.h"

typedef struct
{
    u64 total_kb;
    u64 available_kb;
    u64 used_kb;
    u64 cached_kb;
    u64 swap_total_kb;
    u64 swap_used_kb;
} talos_mem;

bool talos_mem_read(talos_mem *mem);
