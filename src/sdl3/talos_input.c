#include "talos_input.h"
#include "globals.h"
#include "gui/talos_gui.h"
#include "talos_update.h"
#include "vx_fs.h"
#include "vx_io.h"
#include "vx_util.h"
#include "vx_process.h"

#include <SDL3/SDL_events.h>

static void handle_fancontrol(struct talos_ctx *ctx);
static void handle_governor(struct talos_ctx *ctx);

void talos_input_poll(struct talos_ctx *ctx, talos_state *cpu_state)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        talos_gui_event_process(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
            {
                ctx->state &= ~TALOS_RUNTIME_STATE_RUNNING;
                talos_update_stop(cpu_state);
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            {
                ctx->target_fps_ms  = TALOS_TARGET_FPS_30;
                ctx->state         |= TALOS_RUNTIME_STATE_FOCUSED;
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_LOST:
            {
                ctx->target_fps_ms  = TALOS_TARGET_FPS_5;
                ctx->state         &= ~TALOS_RUNTIME_STATE_FOCUSED;
                break;
            }

            case SDL_EVENT_WINDOW_EXPOSED:
            case SDL_EVENT_WINDOW_RESIZED:
            {
                i32 w, h;
                SDL_GetWindowSizeInPixels(ctx->window, &w, &h);
                ctx->width  = (u32) w;
                ctx->height = (u32) h;
                vx_dbglog("Res: %ux%u", ctx->width, ctx->height);
                break;
            }

            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                ctx->width  = (u32) event.window.data1;
                ctx->height = (u32) event.window.data2;
                break;
            }

            case SDL_EVENT_KEY_DOWN:
            {
                switch (event.key.key)
                {
                    case SDLK_F1: ctx->state ^= TALOS_RUNTIME_STATE_ABOUT_WINDOW; break;

                    case SDLK_F2:
                    {
                        ctx->state &= ~TALOS_RUNTIME_STATE_BOOST_FPS;
                        ctx->state ^= TALOS_RUNTIME_STATE_LIMIT_FPS;
                        break;
                    }

                    case SDLK_F3:
                    {
                        ctx->state &= ~TALOS_RUNTIME_STATE_LIMIT_FPS;
                        ctx->state ^= TALOS_RUNTIME_STATE_BOOST_FPS;
                        break;
                    }

                    case SDLK_F4: handle_fancontrol(ctx); break;

                    case SDLK_F5:
                    {
                        handle_governor(ctx);
                        break;
                    }

                    case SDLK_F11:
                    {
                        u32 flags = SDL_GetWindowFlags(ctx->window);
                        if (flags & SDL_WINDOW_FULLSCREEN)
                        {
                            SDL_SetWindowFullscreen(ctx->window, false);
                        }
                        else
                        {
                            SDL_SetWindowFullscreen(ctx->window, true);
                        }
                        break;
                    }

                    case SDLK_F12: ctx->state &= ~TALOS_RUNTIME_STATE_RUNNING; break;
                    case SDLK_G: ctx->state ^= TALOS_RUNTIME_STATE_CPU_GROUPED; break;

                    case SDLK_Q:
                    {
                        ctx->state &= ~TALOS_RUNTIME_STATE_RUNNING;
                        talos_update_stop(cpu_state);
                        break;
                    }

                    default: break;
                }
            }  // key down
            default: break;
        }  // event.type
    }
}

static void handle_fancontrol(struct talos_ctx *ctx)
{
    if (ctx == nullptr || !(ctx->state & TALOS_RUNTIME_STATE_FAN_SUPPORTED))
    {
        return;
    }

    ctx->fan_profile_idx = (ctx->fan_profile_idx + 1) % ctx->fan_profile_count;

    char mode = ctx->fan_profiles[ctx->fan_profile_idx];

    vx_log("Swithcing mode to: %c", mode);

    char  mode_arg[2]  = {mode, '\0'};
    char *fanctl_path  = "/usr/local/bin/talos_fanctl";
    char *spawn_argv[] = {"pkexec", fanctl_path, mode_arg, nullptr};

    struct vx_process  proc = {0};
    struct vx_proc_cfg cfg  = {.flags = VX_PROCESS_FLAGS_BG};

    vx_status status = vx_process_spawn(&proc, spawn_argv[0], spawn_argv, &cfg);

    if (status == VX_OK)
    {
        vx_process_wait(&proc);
        if (proc.exit_code != 0)
        {
            vx_errlog("Failed to spawn talos_fanctl");
        }
    }
}

void talos_system_shutdown(void)
{
    char *power_path   = "/usr/local/bin/talos_power";
    char *spawn_argv[] = {"pkexec", power_path, "poweroff", nullptr};

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
            vx_errlog("Failed to execute talos_power poweroff");
        }
    }
}

void talos_system_reboot(void)
{
    char *power_path   = "/usr/local/bin/talos_power";
    char *spawn_argv[] = {"pkexec", power_path, "reboot", nullptr};

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
            vx_errlog("Failed to execute talos_power reboot");
        }
    }
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

static void handle_governor(struct talos_ctx *ctx)
{
    if (ctx == nullptr)
    {
        return;
    }

    if (ctx->state & TALOS_RUNTIME_STATE_GOV_NORM)
    {
        ctx->state &= ~TALOS_RUNTIME_STATE_GOV_NORM;
        ctx->state |= TALOS_RUNTIME_STATE_GOV_PERF;
        system_set_governor("performance");
    }
    else if (ctx->state & TALOS_RUNTIME_STATE_GOV_PERF)
    {
        ctx->state &= ~TALOS_RUNTIME_STATE_GOV_PERF;
        ctx->state |= TALOS_RUNTIME_STATE_GOV_LIMIT;
        system_set_governor("powersave");
    }
    else
    {
        ctx->state &= ~TALOS_RUNTIME_STATE_GOV_LIMIT;
        ctx->state |= TALOS_RUNTIME_STATE_GOV_NORM;
        system_set_governor("schedutil");
    }
}
