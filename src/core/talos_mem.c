#include "talos_mem.h"
#include "vx_io.h"
#include "vx_string.h"

#include <inttypes.h>
#include <sys/statfs.h>
#include <stdio.h>

#define PROC_MEMINFO "/proc/meminfo"

bool talos_mem_read(talos_mem *mem)
{
    if (mem == nullptr)
    {
        return false;
    }

    FILE *fp = fopen(PROC_MEMINFO, "r");
    if (fp == nullptr)
    {
        VX_ASSERT_LOG("Failed to open %s", PROC_MEMINFO);
        return false;
    }

    char line[VX_BUF_SIZE_128];
    u64  swap_free = 0;

    while (fgets(line, sizeof(line), fp))
    {
        u64 val = 0;
        if (sscanf(line, "%*s %" SCNu64, &val) != 1)
        {
            continue;
        }

        if (strncmp(line, "MemTotal", 8) == 0)
        {
            mem->total_kb = val;
        }
        else if (strncmp(line, "MemAvailable", 12) == 0)
        {
            mem->available_kb = val;
        }
        else if (strncmp(line, "Cached", 6) == 0)
        {
            mem->cached_kb = val;
        }
        else if (strncmp(line, "SwapTotal", 9) == 0)
        {
            mem->swap_total_kb = val;
        }
        else if (strncmp(line, "SwapFree", 8) == 0)
        {
            swap_free = val;
        }
    }

    fclose(fp);

    mem->used_kb      = mem->total_kb - mem->available_kb;
    mem->swap_used_kb = mem->swap_total_kb - swap_free;

    return mem->total_kb > 0;
}

void talos_storage_update(talos_storage_ctx *storage)
{
    if (storage == nullptr)
    {
        return;
    }

    storage->disk_count = 0;

    FILE *f_mounts = fopen("/proc/mounts", "r");
    if (f_mounts == nullptr)
    {
        return;
    }

    char line[VX_BUF_SIZE_512];

    while (fgets(line, sizeof(line), f_mounts) && storage->disk_count < TALOS_MAX_DISKS)
    {
        char dev_buf[VX_BUF_SIZE_64];
        char mount_buf[VX_BUF_SIZE_64];
        char fs_type[VX_BUF_SIZE_32];

        if (sscanf(line, "%63s %63s %31s", dev_buf, mount_buf, fs_type) != 3)
        {
            continue;
        }

        if (strncmp(dev_buf, "/dev/", 5) != 0)
        {
            continue;
        }

        struct statfs vfs;

        if (statfs(mount_buf, &vfs) == 0)
        {
            talos_disk_info *disk = &storage->disks[storage->disk_count];

            snprintf(disk->device, sizeof(disk->device), "%s", dev_buf);
            snprintf(disk->mount_path, sizeof(disk->mount_path), "%s", mount_buf);

            disk->total_bytes = (u64) vfs.f_blocks * vfs.f_frsize;
            disk->free_bytes  = (u64) vfs.f_bavail * vfs.f_frsize;
            disk->used_bytes  = disk->total_bytes - ((u64) vfs.f_bfree * vfs.f_frsize);

            if (disk->total_bytes > 0)
            {
                disk->free_percent = ((f32) disk->free_bytes / (f32) disk->total_bytes) * 100.0f;
            }
            else
            {
                disk->free_percent = 0.0f;
            }

            storage->disk_count++;
        }
    }

    fclose(f_mounts);
}
