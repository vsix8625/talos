#pragma once

#include "talos_state.h"

struct talos_ctx;

void talos_input_poll(struct talos_ctx *ctx, talos_state *cpu_state);

void talos_system_shutdown(void);
void talos_system_reboot(void);
