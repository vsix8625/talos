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

bool talos_cpu_init(talos_cpu *cpu);
void talos_cpu_update(talos_cpu *cpu);
void talos_cpu_destroy(talos_cpu *cpu);
