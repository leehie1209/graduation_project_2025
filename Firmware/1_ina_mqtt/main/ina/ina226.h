#pragma once
#include <stdbool.h>

/* Global data structure */
typedef struct {
    float voltage;   // V
    float current;   // A
    float power;     // W
    bool  valid;
} ina226_data_t;

/* Global instance */
extern ina226_data_t g_ina226_data;

/* API */
void ina226_start_task(void);
