#include "talos_sdl.h"
#include "talos_gl.h"
#include "vx.h"
#include "globals.h"

#include <SDL3/SDL.h>
#include <stdlib.h>

vx_status talos_sdl_init(struct talos_ctx *ctx)
{
    if (ctx == nullptr)
    {
        return VX_ERROR;
    }

    const char *xdg_session_type = getenv("XDG_SESSION_TYPE");
    const char *wayland_display  = getenv("WAYLAND_DISPLAY");

    if ((xdg_session_type && strcmp(xdg_session_type, "wayland") == 0) ||
        (wayland_display && strlen(wayland_display) > 0))
    {
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "wayland", SDL_HINT_OVERRIDE);
        vx_log("Detected Wayland, setting SDL_HINT_VIDEO_DRIVER=wayland");
    }
    else
    {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
        vx_log("Falling back to SDL_HINT_VIDEO_DRIVER=x11");
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        VX_ASSERT_LOG("failed to initialize SDL:  %s", SDL_GetError());
        return VX_ERROR;
    }

    i32 w = 800;
    i32 h = 600;

    SDL_Window *window = SDL_CreateWindow("Talos", w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (window == nullptr)
    {
        VX_ASSERT_LOG("failed to create SDL window");
        return VX_ERROR;
    }

    ctx->width  = w;
    ctx->height = h;
    ctx->window = window;

    if (talos_gl_init(ctx) != VX_OK)
    {
        talos_sdl_shutdown(ctx);
        return VX_ERROR;
    }

    u32 flags = SDL_GetWindowFlags(ctx->window);
    if (!(flags & SDL_WINDOW_FULLSCREEN))
    {
        SDL_SetWindowFullscreen(ctx->window, true);
    }

    ctx->state |= TALOS_RUNTIME_STATE_INITIALIZED;
    return VX_OK;
}

void talos_sdl_shutdown(struct talos_ctx *ctx)
{
    if (ctx == nullptr)
    {
        return;
    }

    if (ctx->gl_context)
    {
        talos_gl_quit(ctx);
    }

    if (ctx->window)
    {
        SDL_DestroyWindow(ctx->window);
    }
    SDL_Quit();

    ctx->state |= TALOS_RUNTIME_STATE_OFF;
}
