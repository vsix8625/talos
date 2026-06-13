#include "talos_update.h"
#include "vx_io.h"
#include <time.h>

static void *update_loop(void *arg)
{
    talos_state *state = (talos_state *) arg;

    struct timespec ts = {.tv_sec = 1, .tv_nsec = 0};

    while (atomic_load(&state->running))
    {
        talos_cpu_update(&state->cpu);
        talos_mem_read(&state->mem);
        talos_temps_update(&state->temps);
        talos_proc_update(&state->proc_list, state->cpu.total_ticks_delta);

        nanosleep(&ts, NULL);
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
