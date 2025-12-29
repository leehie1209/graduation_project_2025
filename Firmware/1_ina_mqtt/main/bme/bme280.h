#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temperature;    // °C
    float humidity;       // %RH
    float pressure_hpa;   // hPa
    bool  valid;
} bme280_data_t;

/* Global data */
extern bme280_data_t g_bme280_data;

/* API */
void bme280_start_task(void);

#ifdef __cplusplus
}
#endif
