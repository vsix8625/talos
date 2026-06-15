#pragma once

#include "vx_defs.h"

typedef struct
{
    u64 last_read_sectors;
    u64 last_write_sectors;
    f32 read_mb_sec;
    f32 write_mb_sec;
} talos_disk_io;

typedef struct
{
    u64 last_rx_bytes;
    u64 last_tx_bytes;
    f32 rx_kb_sec;
    f32 tx_kb_sec;
} talos_net_io;

bool talos_disk_read(talos_disk_io *io, const char *target_dev);
bool talos_net_read(talos_net_io *net, const char *target_iface);
