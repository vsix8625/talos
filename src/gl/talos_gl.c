#include "talos_gl.h"
#include "globals.h"
#include "vx_io.h"
#include "glad.h"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_error.h>

vx_status talos_gl_init(struct talos_ctx *ctx)
{
    if (ctx == nullptr || ctx->window == nullptr)
    {
        return VX_FATAL;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(ctx->window);
    SDL_GL_MakeCurrent(ctx->window, gl_ctx);

    SDL_GL_SetSwapInterval(1);  // vsync

    if (gl_ctx == nullptr)
    {
        VX_ASSERT_LOG("Failed to create openGL context: %s", SDL_GetError());
        return VX_ERROR;
    }

    ctx->gl_context = gl_ctx;

    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
    {
        VX_ASSERT_LOG("Failed to initialize gl loader: %s", SDL_GetError());
        return VX_ERROR;
    }

    const char *driver = SDL_GetCurrentVideoDriver();

    const GLubyte *version  = glGetString(GL_VERSION);
    const GLubyte *vendor   = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);

    vx_log("Driver: %s", driver ? driver : "N/A");
    vx_log("Vendor: %s | %s", vendor, version);
    vx_log("Renderer: %s", renderer);

    return VX_OK;
}

void talos_gl_quit(struct talos_ctx *ctx)
{
    if (ctx == nullptr)
    {
        return;
    }

    if (ctx->gl_context)
    {
        SDL_GL_DestroyContext(ctx->gl_context);
    }

    ctx->gl_context = nullptr;
}

void talos_gl_begin(struct talos_ctx *ctx, SDL_Color color)
{
    if (ctx == nullptr || ctx->window == nullptr)
    {
        return;
    }

    glClearColor(color.r, color.g, color.b, color.a);
    glViewport(0, 0, ctx->width, ctx->height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void talos_gl_end(struct talos_ctx *ctx)
{
    if (ctx == nullptr || ctx->window == nullptr)
    {
        return;
    }

    SDL_GL_SwapWindow(ctx->window);
}
