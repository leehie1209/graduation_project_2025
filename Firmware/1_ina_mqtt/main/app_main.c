#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"

#include "wifi/wifi.h"
#include "mqtt/mqtt.h"

#include "sds/sds011.h"
#include "pms/pms7003.h"
#include "ina/ina226.h"
#include "bme/bme280.h"

#include "sdcard/sdcard.h"

#include "cJSON.h"

/* MQTT client global */
extern esp_mqtt_client_handle_t global_mqtt_client;

static const char *TAG = "APP";

/* ====== CONFIG ====== */
#define APP_PERIOD_MS     4000   // 20s
#define CSV_PATH          "/sdcard/data.csv"

static void csv_create_if_needed(void)
{
    FILE *f = fopen(CSV_PATH, "r");
    if (f) {
        fclose(f);
        return;
    }

    ESP_LOGI(TAG, "Create CSV file + header");

    sdcard_append_line(
        CSV_PATH,
        "pm25_sds,pm10_sds,"
        "pm1_pms,pm25_pms,pm10_pms,"
        "temp,hum,press,"
        "voltage,current,power"
    );
}

static void sdcard_task(void *arg)
{
    ESP_LOGI(TAG, "SD card task started");

    while (!sdcard_is_ready()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    csv_create_if_needed();

    while (1) {

        /* SDS011 */
        float sds_pm25 = g_sds_data.valid ? g_sds_data.pm25 : -1;
        float sds_pm10 = g_sds_data.valid ? g_sds_data.pm10 : -1;

        /* PMS7003 */
        int pms_pm1  = g_pms_data.valid ? g_pms_data.pm1_0 : -1;
        int pms_pm25 = g_pms_data.valid ? g_pms_data.pm2_5 : -1;
        int pms_pm10 = g_pms_data.valid ? g_pms_data.pm10  : -1;

        /* INA226 */
        float volt  = g_ina226_data.valid ? g_ina226_data.voltage : -1;
        float curr  = g_ina226_data.valid ? g_ina226_data.current : -1;
        float power = g_ina226_data.valid ? g_ina226_data.power   : -1;

        /* BME280 (GLOBAL DATA) */
        float temp = g_bme280_data.valid ? g_bme280_data.temperature  : -1;
        float hum  = g_bme280_data.valid ? g_bme280_data.humidity     : -1;
        float pres = g_bme280_data.valid ? g_bme280_data.pressure_hpa : -1;

        /* Append CSV */
        sdcard_append_fmt(
            CSV_PATH,
            "%.1f,%.1f,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f",
            sds_pm25, sds_pm10,
            pms_pm1, pms_pm25, pms_pm10,
            temp, hum, pres,
            volt, curr, power
        );

        ESP_LOGI(TAG, "CSV appended");

        vTaskDelay(pdMS_TO_TICKS(APP_PERIOD_MS));
    }
}

static void mqtt_task(void *arg)
{
    ESP_LOGI(TAG, "MQTT task started");

    while (1) {

        if (global_mqtt_client == NULL) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        cJSON *root = cJSON_CreateObject();

        /* SDS011 */
        if (g_sds_data.valid) {
            cJSON_AddNumberToObject(root, "sds_pm25", g_sds_data.pm25);
            cJSON_AddNumberToObject(root, "sds_pm10", g_sds_data.pm10);
        }

        /* PMS7003 */
        if (g_pms_data.valid) {
            cJSON_AddNumberToObject(root, "pms_pm1",  g_pms_data.pm1_0);
            cJSON_AddNumberToObject(root, "pms_pm25", g_pms_data.pm2_5);
            cJSON_AddNumberToObject(root, "pms_pm10", g_pms_data.pm10);
        }

        /* INA226 */
        if (g_ina226_data.valid) {
            cJSON_AddNumberToObject(root, "voltage", g_ina226_data.voltage);
            cJSON_AddNumberToObject(root, "current", g_ina226_data.current);
            cJSON_AddNumberToObject(root, "power",   g_ina226_data.power);
        }

        /* BME280 */
        if (g_bme280_data.valid) {
            cJSON_AddNumberToObject(root, "temp",  g_bme280_data.temperature);
            cJSON_AddNumberToObject(root, "hum",   g_bme280_data.humidity);
            cJSON_AddNumberToObject(root, "press", g_bme280_data.pressure_hpa);
        }

        char *json = cJSON_PrintUnformatted(root);

        esp_mqtt_client_publish(
            global_mqtt_client,
            "data/value",
            json,
            0,
            1,
            0
        );

        cJSON_free(json);
        cJSON_Delete(root);

        ESP_LOGI(TAG, "MQTT published");

        vTaskDelay(pdMS_TO_TICKS(APP_PERIOD_MS));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "System start");

    /* WiFi + MQTT */
    wifi_init_sta();
    mqtt_app_start();

    /* Sensors */
    sds011_start_task();
    // pms7003_start_task();
    ina226_start_task();
    bme280_start_task();

    /* SD card */
    // sdcard_init();

    /* Application tasks */
    // xTaskCreate(sdcard_task, "sdcard_task", 4096, NULL, 5, NULL);
    xTaskCreate(mqtt_task,   "mqtt_task",   4096, NULL, 5, NULL);
}
