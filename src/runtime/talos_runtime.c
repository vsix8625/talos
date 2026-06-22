#include "talos_runtime.h"
#include "globals.h"
#include "gui/talos_gui.h"
#include "talos_gl.h"
#include "talos_state.h"
#include "talos_update.h"
#include "talos_temp.h"
#include "talos_input.h"
#include "vx_process.h"
#include "vx_time.h"
#include "glad.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>

static void render_stage_dashboard(struct talos_ctx *ctx, void *data);
static void system_set_governor(const char *gov);

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

    // render stage
    talos_init_splash_geometry();

    f32 splash_timer = 0.0f;

    struct talos_render_stage active_stage = {.fn   = talos_render_stage_splash,
                                              .data = &splash_timer};

    ctx->render_dispatch = active_stage.fn;

    talos_rtime_state prev_state = ctx->state;

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
        talos_gl_begin(ctx, bronze_color);
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
            if (!(prev_state & TALOS_RUNTIME_STATE_BOOST_FPS))
            {
                system_set_governor("performance");
            }
        }
        else if (ctx->state & TALOS_RUNTIME_STATE_LIMIT_FPS)
        {
            ctx->target_fps_ms = TALOS_TARGET_FPS_5;
            if (!(prev_state & TALOS_RUNTIME_STATE_LIMIT_FPS))
            {
                system_set_governor("powersave");
            }
        }
        else
        {
            ctx->target_fps_ms = TALOS_TARGET_FPS_30;
            if (prev_state & (TALOS_RUNTIME_STATE_BOOST_FPS | TALOS_RUNTIME_STATE_LIMIT_FPS))
            {
                system_set_governor("schedutil");
            }
        }

        u64 frame_ticks = vx_time_ms() - start_ticks;
        if (frame_ticks < ctx->target_fps_ms)
        {
            SDL_Delay(ctx->target_fps_ms - (u32) frame_ticks);
        }

        prev_state = ctx->state;
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

static void system_set_governor(const char *gov)
{
    if (gov == nullptr)
    {
        return;
    }

    char *power_path   = "/usr/local/bin/talos_power";
    char *spawn_argv[] = {"pkexec", power_path, "governor", (char *) gov, nullptr};

    struct vx_process  proc = {0};
    struct vx_proc_cfg cfg  = {.flags = VX_PROCESS_FLAGS_BG};

    if (!vx_fs_is_exec(power_path))
    {
        vx_errlog("Talos power control helper is not installed at '%s'", power_path);
        return;
    }

    vx_status status = vx_process_spawn(&proc, spawn_argv[0], spawn_argv, &cfg);
    if (status == VX_OK)
    {
        vx_process_wait(&proc);
        if (proc.exit_code != 0)
        {
            vx_errlog("Failed to set governor to '%s'", gov);
        }
    }
}
