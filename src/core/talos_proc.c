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

static u64 g_kb_per_page = 0;

static bool read_proc_stat(i32 pid, talos_process *out)
{
    char path[VX_BUF_SIZE_256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *fp = fopen(path, "r");
    if (fp == nullptr)
    {
        return false;
    }

    char line[VX_BUF_SIZE_1024];
    if (fgets(line, sizeof(line), fp) == nullptr)
    {
        fclose(fp);
        return false;
    }
    fclose(fp);

    out->pid = pid;

    char *open  = strchr(line, '(');
    char *close = strrchr(line, ')');
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
    u64  utime, stime, starttime;

    u64 rss_pages = 0;

    i32 parsed = sscanf(close + 2,
                        "%c %d "
                        "%*d %*d %*d %*d %*d %*d %*d %*d %*d "
                        "%" SCNu64 " %" SCNu64 " "
                        "%*d %*d %*d %*d %*d %*d "
                        "%" SCNu64 " "
                        "%*d "
                        "%" SCNu64,
                        &state,
                        &ppid,
                        &utime,
                        &stime,
                        &starttime,
                        &rss_pages);

    if (parsed != 6)
    {
        return false;
    }

    out->state     = state;
    out->utime     = utime;
    out->stime     = stime;
    out->starttime = starttime;

    out->mem_rss_kb = rss_pages * (u64) g_kb_per_page;

    //----------------------------------------------------------------------------------------------------
    // schedstat
    //----------------------------------------------------------------------------------------------------

    snprintf(path, sizeof(path), "/proc/%d/sched", pid);

    fp = fopen(path, "r");
    if (fp != nullptr)
    {
        char   sched_buf[VX_BUF_SIZE_2048];
        size_t bytes_read = fread(sched_buf, 1, sizeof(sched_buf) - 1, fp);
        fclose(fp);

        if (bytes_read > 0)
        {
            sched_buf[bytes_read] = '\0';

            char *match;

            match = strstr(sched_buf, "se.nr_migrations");
            if (match != nullptr)
            {
                sscanf(match, "se.nr_migrations : %" SCNu64, &out->nr_migrations);
            }

            match = strstr(sched_buf, "nr_voluntary_switches");
            if (match != nullptr)
            {
                sscanf(match, "nr_voluntary_switches : %" SCNu64, &out->switches_voluntary);
            }

            match = strstr(sched_buf, "nr_involuntary_switches");
            if (match != nullptr)
            {
                sscanf(match, "nr_involuntary_switches : %" SCNu64, &out->switches_involuntary);
            }
        }
    }
    else
    {
        out->nr_migrations        = 0;
        out->switches_voluntary   = 0;
        out->switches_involuntary = 0;
    }

    out->runtime_ns = 0;
    snprintf(path, sizeof(path), "/proc/%d/schedstat", pid);
    fp = fopen(path, "r");
    if (fp != nullptr)
    {
        char   schedstat_buf[VX_BUF_SIZE_512];
        size_t bytes_read = fread(schedstat_buf, 1, sizeof(schedstat_buf) - 1, fp);
        fclose(fp);

        if (bytes_read > 0)
        {
            schedstat_buf[bytes_read] = '\0';
            sscanf(schedstat_buf, "%" SCNu64, &out->runtime_ns);
        }
    }

    //----------------------------------------------------------------------------------------------------

    return true;
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

    if (!g_kb_per_page)
    {
        g_kb_per_page = sysconf(_SC_PAGESIZE) / 1024;
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

                p->history_head = (p->history_head + 1) % 60;

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
