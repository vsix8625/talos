#include "gui/talos_gui.h"
#include "runtime/talos_runtime.h"
#include "talos_cpu.h"
#include "talos_sdl.h"
#include "vx.h"
#include "mem.h"
#include "globals.h"

#include <time.h>
#include <inttypes.h>

struct mem_arena *g_talos_global_arena = nullptr;

struct talos_ctx g_talos_ctx = {0};

static i32 talos_init(void)
{
    i32 result = 0;

    if (vx_init() != VX_OK)
    {
        result = VX_EXIT_FAILURE;
    }

#if defined(DEBUG)
    vx_set_debug(true);
#endif

    vx_io_set_prefix(VX_LOG_LEVEL_INFO, "[talos]: ", VX_COLOR_GREEN);

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

    if (talos_sdl_init(&g_talos_ctx) != VX_OK)
    {
        result = VX_EXIT_FAILURE;
    }

    if (!talos_gui_init(g_talos_ctx.window, g_talos_ctx.gl_context))
    {
        result = VX_EXIT_FAILURE;
    }

    return result;
}

static void talos_quit(void)
{
    talos_gui_shutdown();
    talos_sdl_shutdown(&g_talos_ctx);

    mem_arena_destroy(g_talos_global_arena);
    mem_shutdown();
    vx_shutdown();
}

i32 main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    i32 result = talos_init();

    if (result != VX_EXIT_FAILURE)
    {
        talos_runtime(&g_talos_ctx);
    }

    talos_quit();
    return result;
}
