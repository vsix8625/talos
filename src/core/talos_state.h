#pragma once

#include "talos_cpu.h"
#include "talos_mem.h"
#include "talos_temp.h"
#include "talos_proc.h"
#include "talos_io.h"
#include "vx_thread.h"

#include <stdatomic.h>

typedef struct
{
    talos_cpu     cpu;
    talos_mem     mem;
    talos_temps   temps;
    talos_disk_io disk;
    talos_net_io  net;

    atomic_bool      running;
    struct vx_thread thread;
    talos_proc_state proc_state;
} talos_state;
