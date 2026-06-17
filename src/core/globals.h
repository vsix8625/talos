#pragma once

#include "vx_defs.h"
#include <SDL3/SDL_video.h>

//----------------------------------------------------

extern struct mem_arena *g_talos_global_arena;

extern struct talos_ctx g_talos_ctx;

//----------------------------------------------------

typedef enum talos_rtime_state : u32
{
    TALOS_RUNTIME_STATE_OFF          = 0,
    TALOS_RUNTIME_STATE_INITIALIZED  = (1 << 0),
    TALOS_RUNTIME_STATE_RUNNING      = (1 << 2),
    TALOS_RUNTIME_STATE_REBOOT       = (1 << 3),
    TALOS_RUNTIME_STATE_FOCUSED      = (1 << 4),
    TALOS_RUNTIME_STATE_CPU_GROUPED  = (1 << 5),
    TALOS_RUNTIME_STATE_ABOUT_WINDOW = (1 << 6),
} talos_rtime_state;

struct talos_ctx
{
    SDL_Window   *window;
    SDL_GLContext gl_context;

    u32 width, height;

    u64 event_timeout_ms;

    talos_rtime_state state;
};
