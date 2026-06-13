#include "talos_temp.h"
#include "vx_io.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define HWMON_BASE "/sys/class/hwmon"

static const struct
{
    const char *driver;
    const char *label;
} k_temp_labels[] = {
    {"k10temp", "CPU"},
    {"radeon", "GPU"},
    {"amdgpu", "GPU"},
    {"acpitz", "Ambient"},
    {"coretemp", "CPU"},
    {"nvme", "NVMe"},
    {"it8728", "Chassis"},
    {"hp", "Chassis"},
    {"nct6775", "Chassis"},
};
#define K_TEMP_LABELS_COUNT (sizeof(k_temp_labels) / sizeof(k_temp_labels[0]))

static void resolve_label(talos_temp_sensor *s)
{
    for (u32 i = 0; i < K_TEMP_LABELS_COUNT; i++)
    {
        if (strcmp(s->source, k_temp_labels[i].driver) == 0)
        {
            snprintf(s->label, sizeof(s->label), "%s", k_temp_labels[i].label);
            return;
        }
    }
    // fallback to driver name
    strncpy(s->label, s->source, sizeof(s->label) - 1);
    s->label[sizeof(s->label) - 1] = '\0';
}

static f32 read_temp_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == nullptr)
    {
        return 0.0f;
    }

    i32 val = 0;
    fscanf(fp, "%d", &val);
    fclose(fp);

    return (f32) val / 1000.0f;
}

static void read_sensor_name(const char *hwmon_path, char *out, u32 size)
{
    char path[VX_PATH_MAX];
    snprintf(path, sizeof(path), "%s/name", hwmon_path);

    FILE *fp = fopen(path, "r");
    if (fp == nullptr)
    {
        snprintf(out, size, "unknown");
        return;
    }

    if (fgets(out, size, fp) != nullptr)
    {
        size_t len = strlen(out);
        if (len > 0 && out[len - 1] == '\n')
        {
            out[len - 1] = '\0';
        }
    }
    fclose(fp);
}

bool talos_temps_init(talos_temps *temps)
{
    if (temps == nullptr)
    {
        return false;
    }
    memset(temps, 0, sizeof(*temps));

    DIR *dir = opendir(HWMON_BASE);
    if (dir == nullptr)
    {
        VX_ASSERT_LOG("Failed to open %s", HWMON_BASE);
        return false;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != nullptr && temps->count < TALOS_TEMP_MAX_SENSORS)
    {
        if (strncmp(entry->d_name, "hwmon", 5) != 0)
        {
            continue;
        }

        char hwmon_path[VX_BUF_SIZE_512];
        snprintf(hwmon_path, sizeof(hwmon_path), "%s/%s", HWMON_BASE, entry->d_name);

        char temp_input[VX_PATH_MAX];
        snprintf(temp_input, sizeof(temp_input), "%s/temp1_input", hwmon_path);

        FILE *fp = fopen(temp_input, "r");
        if (fp == nullptr)
        {
            continue;  // no temp sensor
        }
        fclose(fp);

        talos_temp_sensor *s = &temps->sensors[temps->count];

        read_sensor_name(hwmon_path, s->source, sizeof(s->source));

        resolve_label(s);

        // read optional thresholds
        char path[VX_PATH_MAX];
        snprintf(path, sizeof(path), "%s/temp1_crit", hwmon_path);
        s->temp_crit = read_temp_file(path);

        snprintf(path, sizeof(path), "%s/temp1_max", hwmon_path);
        s->temp_max = read_temp_file(path);

        temps->count++;
    }

    closedir(dir);

    // read initial values
    talos_temps_update(temps);

    return temps->count > 0;
}

void talos_temps_update(talos_temps *temps)
{
    if (temps == nullptr)
    {
        return;
    }

    DIR *dir = opendir(HWMON_BASE);
    if (dir == nullptr)
    {
        return;
    }

    u32            idx = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != nullptr && idx < temps->count)
    {
        if (strncmp(entry->d_name, "hwmon", 5) != 0)
        {
            continue;
        }

        char path[VX_PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s/temp1_input", HWMON_BASE, entry->d_name);

        FILE *fp = fopen(path, "r");
        if (fp == nullptr)
        {
            continue;
        }
        fclose(fp);

        snprintf(path, sizeof(path), "%s/%s/temp1_input", HWMON_BASE, entry->d_name);
        temps->sensors[idx].temp_c = read_temp_file(path);
        idx++;
    }

    closedir(dir);
}
