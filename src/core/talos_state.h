#pragma once

#include "talos_cpu.h"
#include "talos_mem.h"
#include "talos_temp.h"
#include "vx_thread.h"
#include <stdatomic.h>

typedef struct
{
    talos_cpu        cpu;
    talos_mem        mem;
    talos_temps      temps;
    atomic_bool      running;
    struct vx_thread thread;
} talos_state;
