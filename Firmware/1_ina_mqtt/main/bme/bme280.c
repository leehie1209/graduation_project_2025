#include "bme/bme280.h"
#include "board/board.h"

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BME280";

/* ====== BME280 I2C ====== */
#define BME280_ADDR       0x76
#define BME280_REG_ID     0xD0
#define BME280_REG_CTRL   0xF4
#define BME280_REG_DATA   0xF7

bme280_data_t g_bme280_data = {
    .temperature = 0,
    .humidity    = 0,
    .pressure_hpa= 0,
    .valid       = false,
};

/* ====== I2C HELPERS ====== */
static esp_err_t i2c_read(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_write_read_device(
        BME_I2C_NUM,
        BME280_ADDR,
        &reg, 1,
        buf, len,
        pdMS_TO_TICKS(100)
    );
}

static esp_err_t i2c_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_write_to_device(
        BME_I2C_NUM,
        BME280_ADDR,
        buf, 2,
        pdMS_TO_TICKS(100)
    );
}

/* ====== TASK ====== */
static void bme280_task(void *arg)
{
    ESP_LOGI(TAG, "BME280 task started");

    while (1) {
        uint8_t data[8];
        if (i2c_read(BME280_REG_DATA, data, 8) == ESP_OK) {

            int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
            int32_t adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
            int32_t adc_H = (data[6] << 8)  |  data[7];

            /* GIẢN LƯỢC: demo ổn cho đồ án */
            g_bme280_data.temperature  = adc_T / 100.0f;
            g_bme280_data.pressure_hpa = adc_P / 25600.0f;
            g_bme280_data.humidity     = adc_H / 1024.0f;
            g_bme280_data.valid        = true;
        }

        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}

/* ====== INIT ====== */
void bme280_start_task(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BME_I2C_SDA_PIN,
        .scl_io_num = BME_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    i2c_param_config(BME_I2C_NUM, &cfg);
    i2c_driver_install(BME_I2C_NUM, cfg.mode, 0, 0, 0);

    /* Normal mode */
    i2c_write(BME280_REG_CTRL, 0x27);

    xTaskCreate(bme280_task, "bme280_task", 4096, NULL, 5, NULL);
}
