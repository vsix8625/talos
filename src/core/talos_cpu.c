#include "talos_cpu.h"
#include "globals.h"
#include "mem_arena.h"
#include "vx.h"
#include <fcntl.h>
#include <inttypes.h>

#define PROC_STAT    "/proc/stat"
#define PROC_CPUINFO "/proc/cpuinfo"

static u64 cpu_get_core_mhz_fd(i32 fd)
{
    if (fd < 0)
    {
        return 0;
    }

    char buf[VX_BUF_SIZE_32];

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        return 0;
    }

    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0)
    {
        return 0;
    }

    buf[n] = '\0';

    u64 khz = 0;
    if (sscanf(buf, "%" SCNu64, &khz) != 1)
    {
        return 0;
    }

    return khz / 1000;
}

static bool cpu_read_stat(talos_cpu_stat *stats, u32 count)
{
    FILE *fp = fopen(PROC_STAT, "r");
    if (fp == nullptr)
    {
        vx_errlog("%s: failed to open %s", __func__, PROC_STAT);
        return false;
    }

    char line[VX_BUF_SIZE_256];
    u32  parsed = 0;

    while (fgets(line, sizeof(line), fp) && parsed <= count)
    {
        if (strncmp(line, "cpu", 3) != 0)
        {
            break;
        }

        talos_cpu_stat *s = &stats[parsed];

        u32 offset = (parsed == 0 ? 3 : 4);
        i32 fields = sscanf(line + offset,
                            "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                            " %" SCNu64 " %" SCNu64,
                            &s->user,
                            &s->nice,
                            &s->system,
                            &s->idle,
                            &s->iowait,
                            &s->irq,
                            &s->softirq,
                            &s->steal);

        if (fields != 8)
        {
            VX_ASSERT_LOG("Expected 8 fields, got %d", fields);
            fclose(fp);
            return false;
        }

        parsed++;
    }

    fclose(fp);
    return parsed > 0;
}

static void cpu_read_model(char *out, u32 size)
{
    FILE *fp = fopen(PROC_CPUINFO, "r");
    if (fp == nullptr)
    {
        snprintf(out, size, "Unknown");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, "model name", 10) == 0)
        {
            char *colon = strchr(line, ':');
            if (colon)
            {
                colon += 2;  // skip ': '

                size_t len = strlen(colon);
                if (len > 0 && colon[len - 1] == '\n')
                {
                    colon[len - 1] = '\0';
                }
                snprintf(out, size, "%s", colon);
            }
            break;
        }
    }

    fclose(fp);
}

static f32 cpu_calc_usage(const talos_cpu_stat *prev, const talos_cpu_stat *curr)
{
    u64 prev_idle = prev->idle + prev->iowait;
    u64 curr_idle = curr->idle + curr->iowait;

    u64 prev_total = prev->user + prev->nice + prev->system + prev->idle + prev->iowait +
                     prev->irq + prev->softirq + prev->steal;
    u64 curr_total = curr->user + curr->nice + curr->system + curr->idle + curr->iowait +
                     curr->irq + curr->softirq + curr->steal;

    u64 delta_idle  = curr_idle - prev_idle;
    u64 delta_total = curr_total - prev_total;

    if (delta_total == 0)
    {
        return 0.0f;
    }

    return (1.0f - (f32) delta_idle / (f32) delta_total) * 100.0f;
}

bool talos_cpu_init(talos_cpu *cpu)
{
    if (cpu == nullptr)
    {
        return false;
    }

    cpu->core_count = vx_cpu_get_nproc();
    u32 total       = cpu->core_count + 1;

    cpu->prev     = mem_arena_zalloc(g_talos_global_arena, total * sizeof(talos_cpu_stat));
    cpu->curr     = mem_arena_zalloc(g_talos_global_arena, total * sizeof(talos_cpu_stat));
    cpu->usage    = mem_arena_zalloc(g_talos_global_arena, total * sizeof(f32));
    cpu->freq_mhz = mem_arena_zalloc(g_talos_global_arena, total * sizeof(u64));
    cpu->freq_fds = mem_arena_alloc(g_talos_global_arena, total * sizeof(i32));

    cpu->freq_id_count = (i32) cpu->core_count;
    memset(cpu->freq_fds, -1, total * sizeof(i32));

    if (cpu->prev == nullptr || cpu->curr == nullptr || cpu->usage == nullptr ||
        cpu->freq_mhz == nullptr)
    {
        VX_ASSERT_LOG("Failed to allocate");
        talos_cpu_destroy(cpu);
        return false;
    }

    cpu_read_model(cpu->model, sizeof(cpu->model));
    cpu_read_stat(cpu->curr, total);

    char path[VX_BUF_SIZE_64];
    for (u32 i = 0; i < cpu->core_count; i++)
    {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_cur_freq", i);
        i32 fd = open(path, O_RDONLY);
        if (fd < 0)
        {
            snprintf(
                path, sizeof(path), "/sys/devices/system/cpu/cpu%u/cpufreq/cpuinfo_cur_freq", i);
            fd = open(path, O_RDONLY);
        }
        cpu->freq_fds[i] = fd;
    }

    return true;
}

void talos_cpu_update(talos_cpu *cpu)
{
    if (cpu == nullptr)
    {
        return;
    }

    talos_cpu_stat *tmp = cpu->prev;
    cpu->prev           = cpu->curr;
    cpu->curr           = tmp;

    u32 total = cpu->core_count + 1;
    cpu_read_stat(cpu->curr, total);

    u64 core_freq_sum = 0;

    for (u32 i = 0; i < total; i++)
    {
        cpu->usage[i] = cpu_calc_usage(&cpu->prev[i], &cpu->curr[i]);

        if (i > 0)
        {
            u32 core_id       = i - 1;
            cpu->freq_mhz[i]  = cpu_get_core_mhz_fd(cpu->freq_fds[core_id]);
            core_freq_sum    += cpu->freq_mhz[i];
        }
    }

    cpu->freq_mhz[0] = (cpu->core_count > 0) ? (core_freq_sum / cpu->core_count) : 0;

    u64 prev_total = cpu->prev[0].user + cpu->prev[0].nice + cpu->prev[0].system +
                     cpu->prev[0].idle + cpu->prev[0].iowait + cpu->prev[0].irq +
                     cpu->prev[0].softirq + cpu->prev[0].steal;

    u64 curr_total = cpu->curr[0].user + cpu->curr[0].nice + cpu->curr[0].system +
                     cpu->curr[0].idle + cpu->curr[0].iowait + cpu->curr[0].irq +
                     cpu->curr[0].softirq + cpu->curr[0].steal;

    cpu->total_ticks_delta = curr_total - prev_total;
}

void talos_cpu_destroy(talos_cpu *cpu)
{
    if (cpu == nullptr)
    {
        return;
    }

    for (i32 i = 0; i < cpu->freq_id_count; i++)
    {
        if (cpu->freq_fds[i] >= 0)
        {
            close(cpu->freq_fds[i]);
        }
    }

    cpu->prev     = nullptr;
    cpu->curr     = nullptr;
    cpu->usage    = nullptr;
    cpu->freq_mhz = nullptr;
    cpu->freq_fds = nullptr;
}
