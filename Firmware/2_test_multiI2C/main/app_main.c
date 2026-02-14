#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c/bme280.h"
#include "i2c/i2c_master.h"
#include "i2c/ina226.h"
void sensor_task(void *pv)
{
    float t, h, p;
    float v, i;

    bme280_init();
    ina226_init();

    TickType_t last_bme = 0;
    TickType_t last_ina = 0;

    while (1)
    {
        TickType_t now = xTaskGetTickCount();

        if (now - last_bme >= pdMS_TO_TICKS(10010))
        {
            bme280_read_data(&t, &h, &p);
            printf("[BME280] T=%.2f C, H=%.1f %%, P=%.1f hPa\n", t, h, p);
            last_bme = now;
        }

        if (now - last_ina >= pdMS_TO_TICKS(1000))
        {
            ina226_read(&v, &i);
            printf("[INA226] V=%.3f V, I=%.3f A\n", v, i);
            last_ina = now;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    i2c_master_init();

    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}
