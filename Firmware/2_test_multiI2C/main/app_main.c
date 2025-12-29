#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c/bme280.h"
#include "i2c/i2c_master.h"
#include "i2c/ina226.h"
void bme280_task(void *pv)
{
    float t, h, p;
    bme280_init();

    while (1)
    {
        bme280_read_data(&t, &h, &p);
        printf("[BME280] T=%.2f C, H=%.1f %%, P=%.1f hPa\n", t, h, p);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void ina226_task(void *pv)
{
    float v, i;
    ina226_init();

    while (1)
    {
        ina226_read(&v, &i);
        printf("[INA226] V=%.3f V, I=%.3f A\n", v, i);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    i2c_master_init();

    xTaskCreate(bme280_task, "bme280_task", 4096, NULL, 5, NULL);
    xTaskCreate(ina226_task, "ina226_task", 4096, NULL, 5, NULL);
}
