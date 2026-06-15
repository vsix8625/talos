#include "talos_update.h"
#include "vx_io.h"
#include <time.h>

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

        // NOTE: hardcoded for now
        talos_disk_read(&state->disk, "sda");
        talos_net_read(&state->net, "enp1s0");

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
