#include "talos_runtime.h"
#include "globals.h"
#include "gui/talos_gui.h"
#include "talos_gl.h"
#include "talos_state.h"
#include "talos_update.h"
#include "talos_temp.h"
#include "talos_input.h"
#include "vx_time.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>

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

    talos_state cpu_state = {0};

    talos_cpu_init(&cpu_state.cpu);
    talos_temps_init(&cpu_state.temps);
    talos_update_start(&cpu_state);

    SDL_Color bronze_color = {.r = 54, .g = 47, .b = 40, .a = 255};

    ctx->target_fps_ms  = TALOS_TARGET_FPS_30;
    ctx->state         |= TALOS_RUNTIME_STATE_FOCUSED;

    ctx->fan_state = TALOS_FAN_STATE_BALANCED;

    while (ctx->state & TALOS_RUNTIME_STATE_RUNNING)
    {
        u64 start_ticks = vx_time_ms();

        // For smoother bars we need higher fps and more cpu usage
        // SDL_WaitEventTimeout(nullptr, ctx->event_timeout_ms);
        talos_input_poll(ctx);

        u64 now = vx_time_ms();

        ctx->dt        = (now - ctx->last_time) / 1000.0f;
        ctx->last_time = now;

        // RENDER

        talos_gl_begin(ctx, bronze_color);

        if (ctx->state & TALOS_RUNTIME_STATE_SPLASH)
        {
            // render splash
            ctx->state &= ~TALOS_RUNTIME_STATE_SPLASH;
        }

        // ImGui

        talos_gui_begin_frame();

        u32 ui_idx = atomic_load_explicit(&cpu_state.proc_state.active_idx, memory_order_acquire);
        talos_proc_list *ui_list = &cpu_state.proc_state.buffers[ui_idx];

        talos_ui_render_dashboard(ctx, &cpu_state, ui_list, ctx->width, ctx->height);

        if (ctx->state & TALOS_RUNTIME_STATE_ABOUT_WINDOW)
        {
            // render about
            talos_ui_render_about_popup(ctx);
        }

        talos_gui_render_frame();

        // End ImGui

        talos_gl_end(ctx);

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

    talos_update_stop(&cpu_state);
    talos_cpu_destroy(&cpu_state.cpu);
}
