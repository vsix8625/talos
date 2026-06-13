#include "talos_proc.h"
#include "vx_io.h"
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
            return false;
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

static i32 cmp_cpu(const void *a, const void *b)
{
    const talos_process *pa = (const talos_process *) a;
    const talos_process *pb = (const talos_process *) b;

    if (pb->cpu_usage > pa->cpu_usage)
    {
        return 1;
    }

    if (pb->cpu_usage < pa->cpu_usage)
    {
        return -1;
    }

    return pa->pid - pb->pid;
}

static i32 cmp_mem(const void *a, const void *b)
{
    const talos_process *pa = (const talos_process *) a;
    const talos_process *pb = (const talos_process *) b;

    if (pb->mem_rss_kb > pa->mem_rss_kb)
    {
        return 1;
    }

    if (pb->mem_rss_kb < pa->mem_rss_kb)
    {
        return -1;
    }

    return 0;
}

static i32 cmp_pid(const void *a, const void *b)
{
    const talos_process *pa = (const talos_process *) a;
    const talos_process *pb = (const talos_process *) b;
    return pa->pid - pb->pid;
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

void talos_proc_update(talos_proc_list *list, u64 total_cpu_delta)
{
    if (list == nullptr || total_cpu_delta == 0)
    {
        return;
    }

    memcpy(list->prev, list->procs, sizeof(talos_process) * list->count);
    u32 prev_count = list->count;

    list->count = 0;

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

        if (list->count >= TALOS_PROC_MAX)
        {
            break;
        }

        talos_process *p = &list->procs[list->count];

        if (!read_proc_stat(pid, p))
        {
            continue;
        }
        read_proc_mem(pid, &p->mem_rss_kb);

        p->cpu_usage = 0.0f;
        if (total_cpu_delta > 0)
        {
            for (u32 i = 0; i < prev_count; i++)
            {
                if (list->prev[i].pid == pid)
                {
                    u64 proc_delta =
                        (p->utime + p->stime) - (list->prev[i].utime + list->prev[i].stime);

                    p->cpu_usage = (f32) proc_delta / (f32) total_cpu_delta * 100.0f;

                    break;
                }
            }
        }

        list->count++;
    }
    closedir(dir);
    talos_proc_sort(list);

    list->sort_dirty = false;
}

void talos_proc_sort(talos_proc_list *list)
{
    if (list == nullptr || list->count == 0)
    {
        return;
    }

    switch (list->sort)
    {
        case TALOS_SORT_CPU: qsort(list->procs, list->count, sizeof(talos_process), cmp_cpu); break;
        case TALOS_SORT_MEM: qsort(list->procs, list->count, sizeof(talos_process), cmp_mem); break;
        case TALOS_SORT_PID: qsort(list->procs, list->count, sizeof(talos_process), cmp_pid); break;
    }
}
