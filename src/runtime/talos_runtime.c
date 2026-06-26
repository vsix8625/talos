#include "talos_runtime.h"
#include "globals.h"
#include "gui/talos_gui.h"
#include "mem_arena.h"
#include "talos_gl.h"
#include "talos_state.h"
#include "talos_update.h"
#include "talos_temp.h"
#include "talos_input.h"
#include "vx_time.h"
#include "glad.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>

static void render_stage_dashboard(struct talos_ctx *ctx, void *data);

void talos_runtime(struct talos_ctx *ctx)
{
    if (ctx == nullptr)
    {
        return;
    }

    if (ctx->state & TALOS_RUNTIME_STATE_INITIALIZED)
    {
        ctx->state |= TALOS_RUNTIME_STATE_RUNNING;
    }

    ctx->state |= TALOS_RUNTIME_STATE_SPLASH;

    talos_state cpu_state = {.shutdown_fd = -1};

    talos_cpu_init(&cpu_state.cpu);
    talos_temps_init(&cpu_state.temps);
    talos_proc_init(&cpu_state.proc_state.buffers[0]);

    talos_update_start(&cpu_state);

    SDL_Color bronze_color = {.r = 54, .g = 47, .b = 40, .a = 255};

    SDL_Color renderer_color = bronze_color;

    ctx->target_fps_ms  = TALOS_TARGET_FPS_30;
    ctx->state         |= TALOS_RUNTIME_STATE_FOCUSED;

    ctx->fan_state = TALOS_FAN_STATE_BALANCED;

    //----------------------------------------------------------------------------------------------------
    // read cpuinfo

    talos_cpu_info cpu_info = {0};
    talos_cpu_sysfs_bounds(&cpu_info);
    ctx->cpu_info = mem_arena_zalloc(g_talos_global_arena, sizeof(talos_cpu_info));
    memcpy(ctx->cpu_info, &cpu_info, sizeof(talos_cpu_info));

    //----------------------------------------------------------------------------------------------------

    // render stage
    talos_init_splash_geometry();

    f32 splash_timer = 0.0f;

    struct talos_render_stage active_stage = {.fn   = talos_render_stage_splash,
                                              .data = &splash_timer};

    ctx->render_dispatch = active_stage.fn;

    while (ctx->state & TALOS_RUNTIME_STATE_RUNNING)
    {
        u64 start_ticks = vx_time_ms();

        // SDL_WaitEventTimeout(nullptr, (i32) wait_timeout_ms);
        talos_input_poll(ctx, &cpu_state);

        u64 now = vx_time_ms();

        ctx->dt        = (now - ctx->last_time) / 1000.0f;
        ctx->last_time = now;

        // --------------------------------------------
        // RENDER

        if ((ctx->state & TALOS_RUNTIME_STATE_SHUTTING_DOWN))
        {
            u8 n = 1;

            static bool shutdown_initialized = false;

            if (!shutdown_initialized)
            {
                splash_timer         = 0.0f;
                shutdown_initialized = true;
            }

            renderer_color.r = (renderer_color.r > n) ? renderer_color.r - n : 0;
            renderer_color.g = (renderer_color.g > n) ? renderer_color.g - n : 0;
            renderer_color.b = (renderer_color.b > n) ? renderer_color.b - n : 0;
            renderer_color.a = 255;

            active_stage.fn      = talos_render_stage_splash;
            active_stage.data    = &splash_timer;
            ctx->render_dispatch = active_stage.fn;

            if (renderer_color.r == 0 && renderer_color.g == 0 && renderer_color.b == 0)
            {
                ctx->state &= ~TALOS_RUNTIME_STATE_RUNNING;
            }
        }

        talos_gl_begin(ctx, renderer_color);
        talos_gui_begin_frame();

        ctx->render_dispatch(ctx, active_stage.data);

        // --------------------------------------------
        // render about window
        if (ctx->state & TALOS_RUNTIME_STATE_ABOUT_WINDOW)
        {
            talos_ui_render_about_popup(ctx);
        }

        // --------------------------------------------

        talos_gui_render_frame();
        talos_gl_end(ctx);

        // --------------------------------------------

        if (!(ctx->state & TALOS_RUNTIME_STATE_SPLASH) &&
            !(ctx->state & TALOS_RUNTIME_STATE_SHUTTING_DOWN) &&
            active_stage.fn == talos_render_stage_splash)
        {
            active_stage =
                (struct talos_render_stage) {.fn = render_stage_dashboard, .data = &cpu_state};

            // update to dashboard render
            ctx->render_dispatch = active_stage.fn;
        }

        if (ctx->state & TALOS_RUNTIME_STATE_BOOST_FPS)
        {
            ctx->target_fps_ms = TALOS_TARGET_FPS_60;
        }
        else if (ctx->state & TALOS_RUNTIME_STATE_LIMIT_FPS)
        {
            ctx->target_fps_ms = TALOS_TARGET_FPS_5;
        }
        else
        {
            ctx->target_fps_ms = TALOS_TARGET_FPS_30;
        }

        u64 frame_ticks = vx_time_ms() - start_ticks;
        if (frame_ticks < ctx->target_fps_ms)
        {
            SDL_Delay(ctx->target_fps_ms - (u32) frame_ticks);
        }
    }

    talos_cpu_destroy(&cpu_state.cpu);
    talos_destroy_splash_geometry();
}

static void render_stage_dashboard(struct talos_ctx *ctx, void *data)
{
    if (ctx == nullptr || data == nullptr)
    {
        return;
    }

    talos_state *cpu_state = (talos_state *) data;

    u32 ui_idx = atomic_load_explicit(&cpu_state->proc_state.active_idx, memory_order_acquire);
    talos_proc_list *ui_list = &cpu_state->proc_state.buffers[ui_idx];

    talos_ui_render_dashboard(ctx, cpu_state, ui_list, ctx->width, ctx->height);
}
