#include "talos_update.h"
#include "globals.h"
#include "vx_io.h"

#include <time.h>

static bool net_find_active(char *out_buf, size_t out_len);
static bool disk_find_active(char *out_disk, size_t out_len);

static void *update_loop(void *arg)
{
    talos_state *state = (talos_state *) arg;

    struct timespec tiny_ts = {.tv_sec = 0, .tv_nsec = 10 * 1'000'000};  // 10ms

    const u32 update_interval_ms = 1000;

    while (atomic_load(&state->running))
    {
        talos_cpu_update(&state->cpu);
        talos_mem_read(&state->mem);
        talos_temps_update(&state->temps);
        talos_proc_update(&state->proc_state, state->cpu.total_ticks_delta);

        talos_disk_read(&state->disk, state->disk_device);
        talos_net_read(&state->net, state->net_interface);

        u32 total_sleep_ms = 0;
        while (total_sleep_ms < update_interval_ms && atomic_load(&state->running))
        {
            nanosleep(&tiny_ts, NULL);
            total_sleep_ms += 10;
        }
    }

    return nullptr;
}

bool talos_update_start(talos_state *state)
{
    if (state == nullptr)
    {
        return false;
    }

    if (!net_find_active(state->net_interface, sizeof(state->net_interface)))
    {
        vx_warn("Could not find active net interface. Falling back to loopback.");
        snprintf(state->net_interface, sizeof(state->net_interface), "lo");
    }

    if (!disk_find_active(state->disk_device, sizeof(state->disk_device)))
    {
        snprintf(state->disk_device, sizeof(state->disk_device), "sda");
    }

    atomic_store(&state->running, true);

    state->proc_state.active_idx       = 0;
    state->proc_state.buffers[0].count = 0;
    state->proc_state.buffers[1].count = 0;

    if (vx_thread_create(&state->thread, update_loop, state) != VX_OK)
    {
        vx_errlog("%s: failed to create update thread", __func__);
        atomic_store(&state->running, false);
        return false;
    }

    return true;
}

void talos_update_stop(talos_state *state)
{
    if (state == nullptr)
    {
        return;
    }

    atomic_store(&state->running, false);
    vx_thread_join(&state->thread);
}

static bool net_find_active(char *out_buf, size_t out_len)
{
    if (out_buf == nullptr || out_len == 0)
    {
        return false;
    }

    FILE *f = fopen("/proc/net/route", "r");
    if (f == nullptr)
    {
        return false;
    }

    char line[VX_BUF_SIZE_256];
    char iface[VX_BUF_SIZE_32];
    u64  dest, gateway;
    bool found = false;

    if (fgets(line, sizeof(line), f) == nullptr)
    {
        fclose(f);
        return false;
    }

    while (fgets(line, sizeof(line), f))
    {
        if (sscanf(line, "%31s %lx %lx", iface, &dest, &gateway) == 3)
        {
            if (dest == 0 && gateway != 0)
            {
                strncpy(out_buf, iface, out_len - 1);
                out_buf[out_len - 1] = '\0';

                found = true;
                break;
            }
        }
    }

    fclose(f);
    return found;
}

static bool disk_find_active(char *out_disk, size_t out_len)
{
    if (out_disk == nullptr || out_len == 0)
    {
        return false;
    }

    FILE *f = fopen("/proc/diskstats", "r");
    if (f == nullptr)
    {
        return false;
    }

    char line[VX_BUF_SIZE_256];
    char disk_name[VX_BUF_SIZE_32];
    u32  major, minor;
    u64  reads_completed;

    u64  max_reads                 = 0;
    char best_disk[VX_BUF_SIZE_64] = {0};
    bool found                     = false;

    while (fgets(line, sizeof(line), f))
    {
        if (sscanf(line, "%u %u %31s %lu", &major, &minor, disk_name, &reads_completed) == 4)
        {
            if (strncmp(disk_name, "loop", 4) == 0 || strncmp(disk_name, "ram", 3) == 0)
            {
                continue;
            }

            if (reads_completed > max_reads)
            {
                max_reads = reads_completed;
                strncpy(best_disk, disk_name, sizeof(best_disk) - 1);
                found = true;
            }
        }
    }

    fclose(f);

    if (found)
    {
        strncpy(out_disk, best_disk, out_len - 1);
        out_disk[out_len - 1] = '\0';
    }

    return found;
}
