#include "talos_input.h"
#include "globals.h"
#include "gui/talos_gui.h"
#include "vx_io.h"

#include <SDL3/SDL_events.h>

void talos_input_poll(struct talos_ctx *ctx)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        talos_gui_event_process(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT: ctx->state &= ~TALOS_RUNTIME_STATE_RUNNING; break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            {
                ctx->event_timeout_ms  = TALOS_EVENT_TIME_OUT_MS;
                ctx->state            |= TALOS_RUNTIME_STATE_FOCUSED;
                break;
            }
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            {
                ctx->event_timeout_ms  = TALOS_EVENT_TIME_OUT_MS * 2;
                ctx->state            &= ~TALOS_RUNTIME_STATE_FOCUSED;
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

                    default: break;
                }
            }  // key down
            default: break;
        }  // event.type
    }
}
