#include "talos_proc.h"
#include "vx_io.h"
#include "vx_time.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

static bool is_pid_dir(const char *name)
{
    for (const char *p = name; *p; p++)
    {
        if (!isdigit((u8) *p))
        {
            return false;
        }
    }
    return name[0] != '\0';
}

static bool read_proc_stat(i32 pid, talos_process *out)
{
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *fp = fopen(path, "r");
    if (fp == nullptr)
    {
        return false;
    }

    char line[1024];
    if (fgets(line, sizeof(line), fp) == nullptr)
    {
        fclose(fp);
        return false;
    }
    fclose(fp);

    // parse pid
    out->pid = pid;

    // find comm between ( and )
    char *open  = strchr(line, '(');
    char *close = strrchr(line, ')');  // strrchr handles names with '(' in them
    if (open == nullptr || close == nullptr)
    {
        return false;
    }

    size_t name_len = (size_t) (close - open - 1);

    if (name_len >= TALOS_PROC_NAME_MAX)
    {
        name_len = TALOS_PROC_NAME_MAX - 1;
    }

    memcpy(out->name, open + 1, name_len);
    out->name[name_len] = '\0';

    char state;
    i32  ppid;
    u64  utime, stime;

    i32 parsed = sscanf(close + 2,
                        "%c %d %*d %*d %*d %*d %*d %*d %*d %*d %*d "
                        "%" SCNu64 " %" SCNu64,
                        &state,
                        &ppid,
                        &utime,
                        &stime);

    if (parsed != 4)
    {
        return false;
    }

    out->state = state;
    out->utime = utime;
    out->stime = stime;

    return true;
}

static bool read_proc_mem(i32 pid, u64 *rss_kb)
{
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");
    if (fp == nullptr)
    {
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            sscanf(line + 6, "%" SCNu64, rss_kb);
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}

static i32 cmp_cpu(const void *a, const void *b, void *ctx)
{
    const talos_process *pa  = (const talos_process *) a;
    const talos_process *pb  = (const talos_process *) b;
    talos_sort_direction dir = *(const talos_sort_direction *) ctx;

    i32 result = 0;

    if (pb->cpu_usage > pa->cpu_usage)
    {
        result = 1;
    }
    else if (pb->cpu_usage < pa->cpu_usage)
    {
        result = -1;
    }
    else
    {
        result = pa->pid - pb->pid;
    }

    return (dir == TALOS_SORT_DIR_ASCENDING) ? result : -result;
}

static i32 cmp_mem(const void *a, const void *b, void *ctx)
{
    const talos_process *pa  = (const talos_process *) a;
    const talos_process *pb  = (const talos_process *) b;
    talos_sort_direction dir = *(const talos_sort_direction *) ctx;

    i32 result = 0;
    if (pb->mem_rss_kb > pa->mem_rss_kb)
    {
        result = 1;
    }
    else if (pb->mem_rss_kb < pa->mem_rss_kb)
    {
        result = -1;
    }
    else
    {
        result = pa->pid - pb->pid;
    }

    return (dir == TALOS_SORT_DIR_ASCENDING) ? result : -result;
}

static i32 cmp_pid(const void *a, const void *b, void *ctx)
{
    const talos_process *pa  = (const talos_process *) a;
    const talos_process *pb  = (const talos_process *) b;
    talos_sort_direction dir = *(const talos_sort_direction *) ctx;

    i32 result = pa->pid - pb->pid;

    return (dir == TALOS_SORT_DIR_ASCENDING) ? result : -result;
}

bool talos_proc_init(talos_proc_list *list)
{
    if (list == nullptr)
    {
        return false;
    }

    memset(list, 0, sizeof(*list));
    list->sort = TALOS_SORT_CPU;

    return true;
}

void talos_proc_update(talos_proc_state *state, u64 total_ticks_delta)
{
    if (state == nullptr)
    {
        return;
    }

    u32 front_idx = atomic_load_explicit(&state->active_idx, memory_order_relaxed);
    u32 back_idx  = 1 - front_idx;

    talos_proc_list *next_list = &state->buffers[back_idx];
    talos_proc_list *prev_list = &state->buffers[front_idx];

    next_list->count = 0;

    DIR *dir = opendir("/proc");
    if (dir == nullptr)
    {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (!is_pid_dir(entry->d_name))
        {
            continue;
        }

        i32 pid = atoi(entry->d_name);

        if (next_list->count >= TALOS_PROC_MAX)
        {
            break;
        }

        talos_process *p = &next_list->procs[next_list->count];

        if (!read_proc_stat(pid, p))
        {
            continue;
        }
        read_proc_mem(pid, &p->mem_rss_kb);

        p->cpu_usage = 0.0f;

        for (u32 i = 0; i < prev_list->count; i++)
        {
            if (prev_list->procs[i].pid == pid)
            {
                memcpy(p->cpu_history, prev_list->procs[i].cpu_history, sizeof(p->cpu_history));
                p->history_head = prev_list->procs[i].history_head;

                u64 current_ticks  = p->utime + p->stime;
                u64 previous_ticks = prev_list->procs[i].utime + prev_list->procs[i].stime;

                if (current_ticks >= previous_ticks && total_ticks_delta > 0)
                {
                    u64 proc_ticks_delta = current_ticks - previous_ticks;

                    p->cpu_usage = ((f32) proc_ticks_delta / (f32) total_ticks_delta) * 100.0f;

                    // SCALE TO PER-CORE MODE:
                    // p->cpu_usage *= 4.0f;
                }

                p->cpu_history[p->history_head] = p->cpu_usage;
                p->history_head                 = (p->history_head + 1) % 60;

                break;
            }
        }

        next_list->count++;
    }
    closedir(dir);

    next_list->sort           = prev_list->sort;
    next_list->sort_direction = prev_list->sort_direction;

    talos_proc_sort(next_list);
    next_list->sort_dirty = false;

    atomic_store_explicit(&state->active_idx, back_idx, memory_order_release);
}

void talos_proc_sort(talos_proc_list *list)
{
    if (list == nullptr || list->count == 0)
    {
        return;
    }

    talos_sort_direction dir = list->sort_direction;

    switch (list->sort)
    {
        case TALOS_SORT_CPU:
        {
            qsort_r(list->procs, list->count, sizeof(talos_process), cmp_cpu, &dir);
            break;
        }
        case TALOS_SORT_MEM:
        {
            qsort_r(list->procs, list->count, sizeof(talos_process), cmp_mem, &dir);
            break;
        }
        case TALOS_SORT_PID:
        {
            qsort_r(list->procs, list->count, sizeof(talos_process), cmp_pid, &dir);
            break;
        }
    }
}
