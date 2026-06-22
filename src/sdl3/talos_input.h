#pragma once

struct talos_ctx;

void talos_input_poll(struct talos_ctx *ctx);

void talos_system_shutdown(void);
void talos_system_reboot(void);
