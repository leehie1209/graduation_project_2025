#pragma once

#include <stdbool.h>

typedef struct {
    float pm25;
    float pm10;
    bool valid;
} sds011_data_t;

extern sds011_data_t g_sds_data;

void sds011_start_task(void);
