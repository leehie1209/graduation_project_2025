#include "ina226.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "board/board.h"

static const char *TAG = "INA226";

/* -------- I2C CONFIG -------- */
#define INA_I2C_PORT   I2C_NUM_0
#define INA_SDA_PIN    21
#define INA_SCL_PIN    22
#define INA_ADDR       0x40

/* -------- INA226 PARAM -------- */
#define SHUNT_OHM     0.1f     // R100
#define MAX_CURRENT  0.5f
#define CURRENT_LSB  (MAX_CURRENT / 32768.0f)

/* -------- Registers -------- */
#define REG_CONFIG   0x00
#define REG_BUS      0x02
#define REG_POWER    0x03
#define REG_CURRENT  0x04
#define REG_CALIB    0x05

/* -------- Global data -------- */
ina226_data_t g_ina226_data = {
    .voltage = 0,
    .current = 0,
    .power   = 0,
    .valid   = false,
};

/* -------- I2C helpers -------- */
static esp_err_t reg_write(uint8_t reg, uint16_t val)
{
    uint8_t d[3] = { reg, val >> 8, val & 0xFF };
    return i2c_master_write_to_device(
        INA_I2C_PORT, INA_ADDR, d, 3, pdMS_TO_TICKS(50));
}

static esp_err_t reg_read(uint8_t reg, uint16_t *val)
{
    uint8_t rx[2];
    esp_err_t ret = i2c_master_write_read_device(
        INA_I2C_PORT, INA_ADDR, &reg, 1, rx, 2, pdMS_TO_TICKS(50));
    if (ret != ESP_OK) return ret;
    *val = (rx[0] << 8) | rx[1];
    return ESP_OK;
}

/* -------- INA init -------- */
static void ina226_init(void)
{
    uint16_t calib = (uint16_t)roundf(
        0.00512f / (CURRENT_LSB * SHUNT_OHM));

    reg_write(REG_CONFIG, 0x4127);  // continuous
    reg_write(REG_CALIB, calib);

    ESP_LOGI(TAG, "INA226 initialized");
}

/* -------- TASK -------- */
static void ina226_task(void *arg)
{
    uint16_t raw_v, raw_p;
    int16_t  raw_i;

    while (1) {
        if (reg_read(REG_BUS, &raw_v) == ESP_OK &&
            reg_read(REG_CURRENT, (uint16_t *)&raw_i) == ESP_OK &&
            reg_read(REG_POWER, &raw_p) == ESP_OK)
        {
            g_ina226_data.voltage = raw_v * 1.25e-3f;
            g_ina226_data.current = raw_i * CURRENT_LSB;
            g_ina226_data.power   = raw_p * (CURRENT_LSB * 25.0f);
            g_ina226_data.valid   = true;

            ESP_LOGI(TAG, "V=%.2f I=%.3f P=%.3f",
                     g_ina226_data.voltage,
                     g_ina226_data.current,
                     g_ina226_data.power);
        } else {
            g_ina226_data.valid = false;
            ESP_LOGW(TAG, "INA226 read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* -------- START -------- */
void ina226_start_task(void)
{
    /* I2C init */
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = INA_SDA_PIN,
        .scl_io_num = INA_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(INA_I2C_PORT, &cfg);
    i2c_driver_install(INA_I2C_PORT, cfg.mode, 0, 0, 0);

    ina226_init();

    xTaskCreate(ina226_task, "ina226_task", 3072, NULL, 5, NULL);
    ESP_LOGI(TAG, "INA226 task started");
}
