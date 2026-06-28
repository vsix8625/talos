#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BASE_HWMON_PATH "/sys/class/hwmon"

static int find_universal_pwm_path(char *dest_path, size_t max_len)
{
    DIR *dir = opendir(BASE_HWMON_PATH);
    if (dir == NULL)
    {
        return -1;
    }
    struct dirent *entry;
    char           check_path[512];
    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "hwmon", 5) == 0)
        {
            snprintf(check_path,
                     sizeof(check_path),
                     "%s/%s/pwm1_enable",
                     BASE_HWMON_PATH,
                     entry->d_name);
            if (access(check_path, F_OK) == 0)
            {
                snprintf(dest_path, max_len, "%s", check_path);
                closedir(dir);
                return 0;
            }
        }
    }
    closedir(dir);
    return -1;
}

static int write_pwm_value(const char *path, char value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }
    ssize_t n = write(fd, &value, 1);

    int saved_errno = errno;

    close(fd);
    if (n != 1)
    {
        errno = saved_errno;
        return -1;
    }
    return 0;
}

static int read_pwm_value(const char *path, char *out)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }
    char buf[8] = {0};

    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    close(fd);
    if (n <= 0)
    {
        return -1;
    }
    *out = buf[0];
    return 0;
}

/* Tries every value '0'..'5', restores original value afterward.
 * Prints accepted values as a single line, e.g. "02" or "012". */
static int run_probe(void)
{
    char path[256];
    if (find_universal_pwm_path(path, sizeof(path)) != 0)
    {
        fprintf(stderr, "Talos Helper: No controllable fan interfaces found on this system.\n");
        printf("none\n");
        return 1;
    }

    char original = '2';
    if (read_pwm_value(path, &original) != 0)
    {
        fprintf(stderr,
                "Talos Helper: Failed to read original value of %s: %s\n",
                path,
                strerror(errno));
        /* Not fatal - fall back to a sane default for restore */
    }

    char accepted[8]    = {0};
    int  accepted_count = 0;

    for (char v = '0'; v <= '5'; v++)
    {
        write_pwm_value(path, v); /* return value is unreliable on some drivers
                                      (e.g. hp-wmi can return EINVAL even when
                                      the write is actually applied) - verify
                                      via readback instead of trusting errno */

        char readback = 0;
        if (read_pwm_value(path, &readback) == 0 && readback == v)
        {
            accepted[accepted_count++] = v;
        }
    }

    /* restore original state regardless of outcome */
    write_pwm_value(path, original);

    if (accepted_count == 0)
    {
        fprintf(stderr, "Talos Helper: %s rejected all probed values.\n", path);
        printf("none\n");
        return 1;
    }

    printf("RESULT: %s\n", accepted);
    return 0;
}

static int run_set(const char *profile_arg)
{
    if (profile_arg[1] != '\0')
    {
        fprintf(stderr, "Talos Helper: Invalid profile ID.\n");
        return 1;
    }
    char mode = profile_arg[0];
    if (mode < '0' || mode > '5')
    {
        fprintf(stderr, "Talos Helper: Invalid profile ID (Must be between 0 and 5).\n");
        return 1;
    }

    char path[256];
    if (find_universal_pwm_path(path, sizeof(path)) != 0)
    {
        fprintf(stderr, "Talos Helper: No controllable fan interfaces found on this system.\n");
        return 1;
    }

    write_pwm_value(path, mode); /* return value unreliable on some drivers -
                                     verify via readback below instead */

    char readback = 0;
    if (read_pwm_value(path, &readback) != 0 || readback != mode)
    {
        fprintf(stderr,
                "Talos Helper: Write did not apply (wanted '%c', got '%c'): %s\n",
                mode,
                readback,
                strerror(errno));
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <profile_id 0-5 | probe>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "probe") == 0)
    {
        return run_probe();
    }

    return run_set(argv[1]);
}
