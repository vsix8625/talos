#pragma once

#include "vx_cpu.h"
#include "vx_limits.h"

typedef struct
{
    u64 total_kb;
    u64 available_kb;
    u64 used_kb;
    u64 cached_kb;
    u64 swap_total_kb;
    u64 swap_used_kb;
} talos_mem;

#define TALOS_MAX_DISKS 16

typedef struct talos_disk_info_s
{
    char device[VX_BUF_SIZE_64];
    char mount_path[VX_BUF_SIZE_64];

    u64 total_bytes;
    u64 used_bytes;
    u64 free_bytes;
    f32 free_percent;
} talos_disk_info;

typedef struct talos_storage_ctx_s
{
    talos_disk_info disks[TALOS_MAX_DISKS];
    u32             disk_count;
} talos_storage_ctx;

bool talos_mem_read(talos_mem *mem);
void talos_storage_update(talos_storage_ctx *storage);
