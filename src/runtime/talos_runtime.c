#include "talos_runtime.h"
#include "globals.h"
#include "gui/talos_gui.h"
#include "talos_gl.h"
#include "talos_mem.h"
#include "talos_state.h"
#include "talos_update.h"
#include "talos_temp.h"
#include "talos_input.h"
#include "vx_time.h"
#include <SDL3/SDL_events.h>
#include <time.h>

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

    talos_state cpu_state = {0};

    talos_cpu_init(&cpu_state.cpu);
    talos_temps_init(&cpu_state.temps);
    talos_update_start(&cpu_state);

    SDL_Color bronze_color = {.r = 54, .g = 47, .b = 40, .a = 255};

    ctx->last_time = vx_time_ms();

    while (ctx->state & TALOS_RUNTIME_STATE_RUNNING)
    {
        u64 now        = vx_time_ms();
        ctx->dt        = (now - ctx->last_time) / 1000.0f;
        ctx->last_time = now;

        talos_input_poll(ctx);

        // RENDER

        talos_gl_begin(ctx, bronze_color);

        // ImGui

        talos_gui_begin_frame();

        u32 ui_idx = atomic_load_explicit(&cpu_state.proc_state.active_idx, memory_order_acquire);
        talos_proc_list *ui_list = &cpu_state.proc_state.buffers[ui_idx];

        talos_ui_render_dashboard(&cpu_state, ui_list, ctx->width, ctx->height);

        talos_gui_render_frame();

        // End ImGui

        talos_gl_end(ctx);

        //----------------------------------------------------------------------------------------------------

        const f32 target_dt = 1.0f / 15.0f;
        if (ctx->dt < target_dt)
        {
            f32 sleep_seconds = target_dt - ctx->dt;

            struct timespec ui_ts;
            ui_ts.tv_sec  = (time_t) sleep_seconds;
            ui_ts.tv_nsec = (i64) ((sleep_seconds - (f32) ui_ts.tv_sec) * 1'000'000'000.0f);

            nanosleep(&ui_ts, NULL);
        }
    }

    talos_update_stop(&cpu_state);
    talos_cpu_destroy(&cpu_state.cpu);
}
