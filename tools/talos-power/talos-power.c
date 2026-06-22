#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>

#define MAGIC1       0xfee1dead
#define MAGIC2       0x28121969
#define CMD_POWEROFF 0x4321fedc
#define CMD_REBOOT   0x1234567

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        return 1;
    }

    if (getuid() != 0)
    {
        return 1;
    }

    sync();  // Flush filesystem caches

    if (strcmp(argv[1], "poweroff") == 0)
    {
        syscall(SYS_reboot, MAGIC1, MAGIC2, CMD_POWEROFF, NULL);
    }
    else if (strcmp(argv[1], "reboot") == 0)
    {
        syscall(SYS_reboot, MAGIC1, MAGIC2, CMD_REBOOT, NULL);
    }
    else if (strcmp(argv[1], "governor") == 0)
    {
        if (argc < 3)
        {
            return 1;
        }

        const char *gov = argv[2];

        if (strcmp(gov, "powersave") != 0 && strcmp(gov, "performance") != 0 &&
            strcmp(gov, "schedutil") != 0)
        {
            return 1;
        }

        char path[256];
        int  all_ok     = 1;
        int  core_count = get_nprocs();

        for (int i = 0; i < core_count; i++)
        {
            snprintf(
                path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);

            int fd = open(path, O_WRONLY);
            if (fd < 0)
            {
                break;  // no more cores
            }

            ssize_t n = write(fd, gov, strlen(gov));
            close(fd);
            if (n < 0)
            {
                all_ok = 0;
            }
        }
        return all_ok ? 0 : 1;
    }

    return 0;
}
