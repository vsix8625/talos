#pragma once
#include "vx_cpu.h"

#define TALOS_PROC_NAME_MAX    64
#define TALOS_PROC_MAX         512
#define TALOS_PROC_HISTORY_MAX 60

typedef struct talos_process_s
{
    i32  pid;
    char name[TALOS_PROC_NAME_MAX];
    char state;
    u64  mem_rss_kb;
    u64  utime;
    u64  stime;
    u64  starttime;
    f32  cpu_usage;

    // sched
    u64 runtime_ns;
    u64 nr_migrations;
    u64 switches_voluntary;
    u64 switches_involuntary;

    f32 cpu_history[TALOS_PROC_HISTORY_MAX];
    i32 history_head;
} talos_process;

typedef enum
{
    TALOS_SORT_CPU,
    TALOS_SORT_MEM,
    TALOS_SORT_PID,
} talos_sort_mode;

typedef enum
{
    TALOS_SORT_DIR_DESCENDING = 0,
    TALOS_SORT_DIR_ASCENDING  = 1
} talos_sort_direction;

typedef struct
{
    talos_process        procs[TALOS_PROC_MAX];
    talos_process        prev[TALOS_PROC_MAX];
    u32                  count;
    talos_sort_mode      sort;
    talos_sort_direction sort_direction;
    bool                 sort_dirty;
} talos_proc_list;

typedef struct
{
    talos_proc_list buffers[2];
    _Atomic u32     active_idx;
} talos_proc_state;

bool talos_proc_init(talos_proc_list *list);
void talos_proc_update(talos_proc_state *state, u64 total_ticks_delta);
void talos_proc_sort(talos_proc_list *list);
