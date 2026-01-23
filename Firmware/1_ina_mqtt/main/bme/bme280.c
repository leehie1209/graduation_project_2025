#include "bme/bme280.h"
#include "board/board.h"

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BME280";

/* ===== I2C ===== */
#define BME280_ADDR        0x76
#define REG_ID             0xD0
#define REG_RESET          0xE0
#define REG_CTRL_HUM       0xF2
#define REG_STATUS         0xF3
#define REG_CTRL_MEAS      0xF4
#define REG_CONFIG         0xF5
#define REG_PRESS_MSB      0xF7

/* ===== GLOBAL DATA ===== */
bme280_data_t g_bme280_data = {
    .temperature = 0,
    .humidity    = 0,
    .pressure_hpa= 0,
    .valid       = false
};

/* ===== CALIBRATION ===== */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4, dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static bme280_calib_t calib;
static int32_t t_fine;

/* ===== I2C HELPERS ===== */
static esp_err_t i2c_read(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_master_write_read_device(
        BME_I2C_NUM, BME280_ADDR,
        &reg, 1, buf, len,
        pdMS_TO_TICKS(100)
    );
}

static esp_err_t i2c_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_write_to_device(
        BME_I2C_NUM, BME280_ADDR,
        buf, 2, pdMS_TO_TICKS(100)
    );
}

/* ===== READ CALIB ===== */
static void bme280_read_calib(void)
{
    uint8_t buf[26];
    i2c_read(0x88, buf, 26);

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

    uint8_t hbuf[7];
    i2c_read(0xE1, hbuf, 7);

    calib.dig_H2 = (hbuf[1] << 8) | hbuf[0];
    calib.dig_H3 = hbuf[2];
    calib.dig_H4 = (hbuf[3] << 4) | (hbuf[4] & 0x0F);
    calib.dig_H5 = (hbuf[5] << 4) | (hbuf[4] >> 4);
    calib.dig_H6 = (int8_t)hbuf[6];
}

/* ===== COMPENSATION (BOSCH) ===== */
static float compensate_T(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * calib.dig_T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - calib.dig_T1) * ((adc_T >> 4) - calib.dig_T1)) >> 12) * calib.dig_T3) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) / 25600.0f;
}

static float compensate_P(int32_t adc_P)
{
    int64_t var1 = (int64_t)t_fine - 128000;
    int64_t var2 = var1 * var1 * calib.dig_P6;
    var2 += (var1 * calib.dig_P5) << 17;
    var2 += ((int64_t)calib.dig_P4) << 35;
    var1 = ((var1 * var1 * calib.dig_P3) >> 8) + ((var1 * calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * calib.dig_P1) >> 33;

    if (var1 == 0) return 0;

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (calib.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = (calib.dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + ((int64_t)calib.dig_P7 << 4);

    return p / 25600.0f;
}

static float compensate_H(int32_t adc_H)
{
    int32_t v = t_fine - 76800;
    v = (((((adc_H << 14) - (calib.dig_H4 << 20) - (calib.dig_H5 * v)) + 16384) >> 15) *
        (((((((v * calib.dig_H6) >> 10) * (((v * calib.dig_H3) >> 11) + 32768)) >> 10) + 2097152) *
          calib.dig_H2 + 8192) >> 14));
    v -= (((v >> 15) * (v >> 15)) >> 7) * calib.dig_H1 >> 4;
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return (v >> 12) / 1024.0f;
}

/* ===== TASK ===== */
static void bme280_task(void *arg)
{
    while (1) {
        uint8_t data[8];
        if (i2c_read(REG_PRESS_MSB, data, 8) == ESP_OK) {

            int32_t adc_P = (data[0]<<12)|(data[1]<<4)|(data[2]>>4);
            int32_t adc_T = (data[3]<<12)|(data[4]<<4)|(data[5]>>4);
            int32_t adc_H = (data[6]<<8)|data[7];

            g_bme280_data.temperature  = compensate_T(adc_T);
            g_bme280_data.pressure_hpa = compensate_P(adc_P);
            g_bme280_data.humidity     = compensate_H(adc_H);
            g_bme280_data.valid        = true;
        }
        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}

/* ===== INIT ===== */
void bme280_start_task(void)
{
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BME_I2C_SDA_PIN,
        .scl_io_num = BME_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    i2c_param_config(BME_I2C_NUM, &cfg);
    i2c_driver_install(BME_I2C_NUM, cfg.mode, 0, 0, 0);

    bme280_read_calib();

    i2c_write(REG_CTRL_HUM, 0x01);   // oversampling x1
    i2c_write(REG_CTRL_MEAS, 0x27);  // normal mode

    xTaskCreate(bme280_task, "bme280_task", 4096, NULL, 5, NULL);
}
