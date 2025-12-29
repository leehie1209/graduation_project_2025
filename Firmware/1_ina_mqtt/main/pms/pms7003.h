#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10;
    bool valid;
} pms7003_data_t;

extern pms7003_data_t g_pms_data;

/**
 * @brief Init UART + start PMS7003 task
 */
void pms7003_start_task(void);
