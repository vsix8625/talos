#include "talos_cpu.h"
#include "talos_mem.h"
#include "talos_state.h"
#include "talos_update.h"
#include "talos_temp.h"
#include "vx.h"
#include "mem.h"
#include "globals.h"

#include <time.h>
#include <inttypes.h>

struct mem_arena *g_talos_global_arena = nullptr;

i32 main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    i32 result = 0;

    if (vx_init() != VX_OK)
    {
        result = VX_EXIT_FAILURE;
    }

    if (!mem_init())
    {
        result = VX_EXIT_FAILURE;
    }

    g_talos_global_arena = mem_arena_create("global-arena", VX_MiB(4));

    if (g_talos_global_arena == nullptr)
    {
        VX_ASSERT_LOG("Failed to create global-arena");
        result = VX_EXIT_FAILURE;
    }

    if (result != VX_EXIT_FAILURE)
    {
        talos_state state = {0};

        talos_cpu_init(&state.cpu);
        talos_temps_init(&state.temps);
        talos_update_start(&state);

        // render loop will go here
        struct timespec ts = {.tv_sec = 1, .tv_nsec = 0};
        for (i32 i = 0; i < 20; i++)
        {
            nanosleep(&ts, NULL);
            vx_printf("\033[2J\033[H");
            vx_log("Aggregate: %.1f%%", state.cpu.usage[0]);
            vx_log("RAM: %" PRIu64 " MB used", state.mem.used_kb / 1024);
            vx_log("CPU temp: %.1f°C", state.temps.sensors[1].temp_c);
        }

        talos_update_stop(&state);
        talos_cpu_destroy(&state.cpu);
    }

    mem_arena_destroy(g_talos_global_arena);
    mem_shutdown();
    vx_shutdown();
    return result;
}
