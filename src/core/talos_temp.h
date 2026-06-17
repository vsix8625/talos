#pragma once

#include "vx_cpu.h"

#define TALOS_TEMP_LABEL_MAX   32
#define TALOS_TEMP_MAX_SENSORS 8
#define TALOS_TEMP_NAME_MAX    32

typedef struct
{
    char source[TALOS_TEMP_NAME_MAX];  // driver name: k10temp, radeon, acpitz
    char label[TALOS_TEMP_LABEL_MAX];
    char hwmon_path[512];
    f32  temp_c;     // current temp
    f32  temp_crit;  // critical threshold, 0 if unavailable
    f32  temp_max;   // max threshold, 0 if unavailable

    u32  updates_since_change;
    bool is_frozen;
} talos_temp_sensor;

typedef struct
{
    talos_temp_sensor sensors[TALOS_TEMP_MAX_SENSORS];
    u32               count;
} talos_temps;

bool talos_temps_init(talos_temps *temps);
void talos_temps_update(talos_temps *temps);
