#pragma once

#include "vx_defs.h"
#include <SDL3/SDL_pixels.h>

struct talos_ctx;

vx_status talos_gl_init(struct talos_ctx *ctx);
void      talos_gl_quit(struct talos_ctx *ctx);

void talos_gl_begin(struct talos_ctx *ctx, SDL_Color color);
void talos_gl_end(struct talos_ctx *ctx);
