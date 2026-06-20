#pragma once

#include "talos_state.h"

struct talos_ctx;

void talos_runtime(struct talos_ctx *ctx);

void talos_ui_render_dashboard(struct talos_ctx *ctx,
                               talos_state      *state,
                               talos_proc_list  *proc_list,
                               u32               width,
                               u32               height);

void talos_ui_render_about_popup(struct talos_ctx *ctx);

void talos_init_splash_geometry(void);
void talos_destroy_splash_geometry(void);
void talos_render_stage_splash(struct talos_ctx *ctx, void *data);
