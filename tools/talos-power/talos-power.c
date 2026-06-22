#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

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

    return 0;
}
