#pragma once

#include "vx.h"
#include "vx_cpu.h"

#define TALOS_CPU_NAME_MAX 64

typedef struct
{
    u64 user;
    u64 nice;
    u64 system;
    u64 idle;
    u64 iowait;
    u64 irq;
    u64 softirq;
    u64 steal;
} talos_cpu_stat;

typedef struct talos_cpu_s
{
    u32             core_count;
    talos_cpu_stat *prev;   // [core_count + 1] +1 for aggregate
    talos_cpu_stat *curr;   // [core_count + 1]
    f32            *usage;  // [core_count + 1] percentage per core + aggregate
    u64            *freq_mhz;
    i32            *freq_fds;
    i32             freq_id_count;
    char            model[TALOS_CPU_NAME_MAX];
    u64             total_ticks_delta;
    char            governor[VX_BUF_SIZE_32];
} talos_cpu;

typedef struct talos_cpu_info_s
{
    char model_name[VX_BUF_SIZE_128];
    char version[VX_BUF_SIZE_256];
    char vendor_id[VX_BUF_SIZE_64];
    char architecture[VX_BUF_SIZE_32];
    u32  total_cores;
    u32  threads_per_core;
    f32  max_mhz;
    f32  min_mhz;
    char cache_l1d[VX_BUF_SIZE_32];
    char cache_l1i[VX_BUF_SIZE_32];
    char cache_l2[VX_BUF_SIZE_32];
    char cache_l3[VX_BUF_SIZE_32];
    u32  numa_nodes;
    char flags[VX_BUF_SIZE_512];
} talos_cpu_info;

bool talos_cpu_init(talos_cpu *cpu);
void talos_cpu_update(talos_cpu *cpu);
void talos_cpu_destroy(talos_cpu *cpu);

void talos_cpu_sysfs_bounds(talos_cpu_info *info);
