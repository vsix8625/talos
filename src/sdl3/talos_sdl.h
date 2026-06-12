#pragma once
#include "vx_defs.h"

struct talos_ctx;

vx_status talos_sdl_init(struct talos_ctx *ctx);
void      talos_sdl_shutdown(struct talos_ctx *ctx);
