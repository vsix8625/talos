#include "talos_runtime.h"
#include "globals.h"
#include "talos_gl.h"
#include "vx_time.h"
#include <SDL3/SDL_events.h>

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

    SDL_Event event;
    SDL_Color bg_color = {.r = 50, .g = 50, .b = 120, .a = 255};

    while (ctx->state & TALOS_RUNTIME_STATE_RUNNING)
    {
        u64 now = vx_time_ns();

        ctx->dt        = (now - ctx->last_time) / 1000.0f;
        ctx->last_time = now;

        // input poll fn
        // NOTE: just a simple quit for now

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                {
                    ctx->state &= ~TALOS_RUNTIME_STATE_RUNNING;
                    break;
                }
                default:
                    break;
            }
        }  // end of tmp poll event

        // RENDER

        talos_gl_begin(ctx, bg_color);

        // imgui goes here

        talos_gl_end(ctx);
    }
}
