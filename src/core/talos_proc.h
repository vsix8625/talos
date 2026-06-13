#pragma once
#include "vx_cpu.h"

#define TALOS_PROC_NAME_MAX 64
#define TALOS_PROC_MAX      512

typedef struct
{
    i32  pid;
    char name[TALOS_PROC_NAME_MAX];
    char state;
    u64  mem_rss_kb;
    u64  utime;
    u64  stime;
    f32  cpu_usage;
} talos_process;

typedef enum
{
    TALOS_SORT_CPU,
    TALOS_SORT_MEM,
    TALOS_SORT_PID,
} talos_sort_mode;

typedef struct
{
    talos_process   procs[TALOS_PROC_MAX];
    talos_process   prev[TALOS_PROC_MAX];
    u32             count;
    talos_sort_mode sort;
    bool            sort_dirty;
} talos_proc_list;

bool talos_proc_init(talos_proc_list *list);
void talos_proc_update(talos_proc_list *list, u64 total_cpu_delta);
void talos_proc_sort(talos_proc_list *list);
