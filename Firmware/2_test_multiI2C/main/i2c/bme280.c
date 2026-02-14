#include "bme280.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c/i2c_master.h"

static const char *TAG = "BME280";

/* ===== PRIVATE DEFINITIONS ===== */
#define REG_RESET      0xE0
#define REG_CTRL_HUM   0xF2
#define REG_STATUS     0xF3
#define REG_CTRL_MEAS  0xF4
#define REG_CONFIG     0xF5
#define REG_DATA       0xF7
#define REG_CALIB00    0x88
#define REG_CALIB26    0xE1

static esp_err_t bme280_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    return i2c_master_write_to_device(
        I2C_MASTER_PORT, BME280_ADDR,
        buf, 2, pdMS_TO_TICKS(100)
    );
}

static esp_err_t bme280_read_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_master_write_read_device(
        I2C_MASTER_PORT, BME280_ADDR,
        &reg, 1, buf, len, pdMS_TO_TICKS(100)
    );
}

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5;
    int16_t  dig_P6, dig_P7, dig_P8, dig_P9;

    uint8_t  dig_H1, dig_H3;
    int16_t  dig_H2, dig_H4, dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static bme280_calib_t calib;
static int32_t t_fine;

static void bme280_read_calibration(void)
{
    uint8_t buf[26];
    bme280_read_reg(REG_CALIB00, buf, 26);

    calib.dig_T1 = (buf[1] << 8) | buf[0];
    calib.dig_T2 = (buf[3] << 8) | buf[2];
    calib.dig_T3 = (buf[5] << 8) | buf[4];

    calib.dig_P1 = (buf[7] << 8) | buf[6];
    calib.dig_P2 = (buf[9] << 8) | buf[8];
    calib.dig_P3 = (buf[11]<< 8) | buf[10];
    calib.dig_P4 = (buf[13]<< 8) | buf[12];
    calib.dig_P5 = (buf[15]<< 8) | buf[14];
    calib.dig_P6 = (buf[17]<< 8) | buf[16];
    calib.dig_P7 = (buf[19]<< 8) | buf[18];
    calib.dig_P8 = (buf[21]<< 8) | buf[20];
    calib.dig_P9 = (buf[23]<< 8) | buf[22];

    calib.dig_H1 = buf[25];

    bme280_read_reg(REG_CALIB26, buf, 7);
    calib.dig_H2 = (buf[1] << 8) | buf[0];
    calib.dig_H3 = buf[2];
    calib.dig_H4 = (buf[3] << 4) | (buf[4] & 0x0F);
    calib.dig_H5 = (buf[5] << 4) | (buf[4] >> 4);
    calib.dig_H6 = (int8_t)buf[6];
}

void bme280_init(void)
{
    bme280_write_reg(REG_RESET, 0xB6);
    vTaskDelay(pdMS_TO_TICKS(100));

    bme280_read_calibration();

    bme280_write_reg(REG_CTRL_HUM, 0x01); // Hum x1
    bme280_write_reg(REG_CTRL_MEAS, 0x27); // Temp+Press x1, normal
    bme280_write_reg(REG_CONFIG, 0xA0); // standby 1000ms, filter off

    ESP_LOGI(TAG, "BME280 init done");
}

void bme280_read_data(float *temp, float *humidity, float *pressure)
{
    uint8_t data[8];
    bme280_read_reg(REG_DATA, data, 8);

    int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    int32_t adc_H = (data[6] << 8)  |  data[7];

    /* ================= TEMPERATURE ================= */
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) *
                    calib.dig_T2) >> 11;

    int32_t var2 = (((((adc_T >> 4) - calib.dig_T1) *
                      ((adc_T >> 4) - calib.dig_T1)) >> 12) *
                    calib.dig_T3) >> 14;

    t_fine = var1 + var2;
    *temp = (t_fine * 5 + 128) / 25600.0f;

    /* ================= PRESSURE ================= */
    int64_t p;
    int64_t pvar1 = ((int64_t)t_fine) - 128000;
    int64_t pvar2 = pvar1 * pvar1 * calib.dig_P6;

    pvar2 = pvar2 + ((pvar1 * calib.dig_P5) << 17);
    pvar2 = pvar2 + (((int64_t)calib.dig_P4) << 35);
    pvar1 = ((pvar1 * pvar1 * calib.dig_P3) >> 8) +
            ((pvar1 * calib.dig_P2) << 12);
    pvar1 = (((((int64_t)1) << 47) + pvar1) * calib.dig_P1) >> 33;

    if (pvar1 == 0) {
        *pressure = 0;
    } else {
        p = 1048576 - adc_P;
        p = (((p << 31) - pvar2) * 3125) / pvar1;
        pvar1 = (calib.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
        pvar2 = (calib.dig_P8 * p) >> 19;
        p = ((p + pvar1 + pvar2) >> 8) + (((int64_t)calib.dig_P7) << 4);
        *pressure = p / 25600.0f;   // hPa
    }

    /* ================= HUMIDITY ================= */
    int32_t hvar = t_fine - 76800;

    hvar = (((((adc_H << 14) -
               (((int32_t)calib.dig_H4) << 20) -
               (((int32_t)calib.dig_H5) * hvar)) + 16384) >> 15) *
            (((((((hvar * calib.dig_H6) >> 10) *
                (((hvar * calib.dig_H3) >> 11) + 32768)) >> 10) + 2097152) *
              calib.dig_H2 + 8192) >> 14));

    hvar = hvar -
           (((((hvar >> 15) * (hvar >> 15)) >> 7) *
             calib.dig_H1) >> 4);

    if (hvar < 0) hvar = 0;
    if (hvar > 419430400) hvar = 419430400;

    *humidity = (hvar >> 12) / 1024.0f;   // %
}

