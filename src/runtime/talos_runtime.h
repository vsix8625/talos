#pragma once

#include "talos_state.h"

struct talos_ctx;

void talos_runtime(struct talos_ctx *ctx);

void talos_ui_render_dashboard(talos_state *state, u32 width, u32 height);
