#pragma once

#include "vx_defs.h"
#include <SDL3/SDL_video.h>

//----------------------------------------------------

extern struct mem_arena *g_talos_global_arena;

extern struct talos_ctx g_talos_ctx;

//----------------------------------------------------

typedef enum talos_rtime_state : u32
{
    TALOS_RUNTIME_STATE_OFF           = 0,
    TALOS_RUNTIME_STATE_INITIALIZED   = (1 << 0),
    TALOS_RUNTIME_STATE_RUNNING       = (1 << 2),
    TALOS_RUNTIME_STATE_REBOOT        = (1 << 3),
    TALOS_RUNTIME_STATE_FOCUSED       = (1 << 4),
    TALOS_RUNTIME_STATE_CPU_GROUPED   = (1 << 5),
    TALOS_RUNTIME_STATE_ABOUT_WINDOW  = (1 << 6),
    TALOS_RUNTIME_STATE_SPLASH        = (1 << 7),
    TALOS_RUNTIME_STATE_LIMIT_FPS     = (1 << 8),
    TALOS_RUNTIME_STATE_BOOST_FPS     = (1 << 9),
    TALOS_RUNTIME_STATE_FAN_SUPPORTED = (1 << 10),
} talos_rtime_state;

#define TALOS_FAN_STATE_BALANCED    (1 << 0)
#define TALOS_FAN_STATE_PERFORMANCE (1 << 1)
#define TALOS_FAN_STATE_SILENT      (1 << 2)

#define TALOS_FAN_STATE_MASK                                                                       \
    (TALOS_FAN_STATE_BALANCED | TALOS_FAN_STATE_PERFORMANCE | TALOS_FAN_STATE_SILENT)

typedef void (*talos_render_stage_fn)(struct talos_ctx *ctx, void *data);

struct talos_render_stage
{
    talos_render_stage_fn fn;

    void *data;
};

struct talos_ctx
{
    talos_render_stage_fn render_dispatch;

    SDL_Window   *window;
    SDL_GLContext gl_context;

    u32 width, height;

    f32 dt;
    u64 last_time;

    u32 target_fps_ms;

    u32 fan_state;

    char fan_profiles[6];
    i32  fan_profile_count;
    i32  fan_profile_idx;  // which one is currently active

    talos_rtime_state state;
};

#define TALOS_TARGET_FPS_60 16
#define TALOS_TARGET_FPS_30 32
#define TALOS_TARGET_FPS_15 64
#define TALOS_TARGET_FPS_5  200

#define TALOS_HISTORY_COUNT 60
