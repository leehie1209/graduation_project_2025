#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include <math.h>

#define INA_I2C_PORT   I2C_NUM_0
#define INA_ADDR      0x40

#define INA_SDA_PIN   21
#define INA_SCL_PIN   22

#define SHUNT_OHM     0.1f    // R100
#define MAX_CURRENT  0.5f    // A
#define CURRENT_LSB  (MAX_CURRENT / 32768.0f)

#define REG_CONFIG   0x00
#define REG_BUS      0x02
#define REG_POWER    0x03
#define REG_CURRENT  0x04
#define REG_CALIB    0x05

static esp_err_t ina_write(uint8_t reg, uint16_t val)
{
    uint8_t data[3] = { reg, val >> 8, val & 0xFF };
    return i2c_master_write_to_device(
        INA_I2C_PORT, INA_ADDR, data, 3, pdMS_TO_TICKS(100));
}

static esp_err_t ina_read(uint8_t reg, uint16_t *val)
{
    uint8_t rx[2];
    esp_err_t ret = i2c_master_write_read_device(
        INA_I2C_PORT, INA_ADDR, &reg, 1, rx, 2, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;
    *val = (rx[0] << 8) | rx[1];
    return ESP_OK;
}

void ina226_init(void)
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

    /* Calibration */
    uint16_t calib = (uint16_t)roundf(
        0.00512f / (CURRENT_LSB * SHUNT_OHM));

    /* Continuous mode */
    ina_write(REG_CONFIG, 0x4127);
    ina_write(REG_CALIB, calib);
}

void ina226_read(float *voltage_v,
                 float *current_a,
                 float *power_w)
{
    uint16_t raw_v, raw_p;
    int16_t  raw_i;

    ina_read(REG_BUS, &raw_v);
    ina_read(REG_CURRENT, (uint16_t *)&raw_i);
    ina_read(REG_POWER, &raw_p);

    *voltage_v = raw_v * 1.25e-3f;
    *current_a = raw_i * CURRENT_LSB;
    *power_w   = raw_p * (CURRENT_LSB * 25.0f);
}

void app_main(void)
{
    float v, i, p;

    ina226_init();

    while (1) {
        ina226_read(&v, &i, &p);
        printf("V=%.2f V | I=%.3f A | P=%.3f W\n", v, i, p);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
