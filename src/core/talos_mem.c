#include "talos_mem.h"
#include "vx_io.h"
#include "vx_string.h"

#include <inttypes.h>
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

    char line[128];
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
