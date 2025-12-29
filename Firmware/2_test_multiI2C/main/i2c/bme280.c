#include "driver/i2c.h"
#include "esp_log.h"
#include "i2c/i2c_master.h"   

static const char *TAG = "BME280";

#define BME280_ADDR  0x76

// Ghi 1 byte vào thanh ghi
static esp_err_t bme280_write(uint8_t reg, uint8_t data)
{
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_write_to_device(I2C_MASTER_PORT, BME280_ADDR, write_buf, 2, 1000 / portTICK_PERIOD_MS);
}

// Đọc n byte từ thanh ghi
static esp_err_t bme280_read(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_PORT, BME280_ADDR, &reg, 1, buf, len, 1000 / portTICK_PERIOD_MS);
}

void bme280_init()
{
    // Soft reset
    bme280_write(0xE0, 0xB6);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Cài đặt oversampling đơn giản
    bme280_write(0xF2, 0x01); // humidity x1
    bme280_write(0xF4, 0x27); // temp & pressure oversampling x1, mode = normal
}

void bme280_read_data(float *temp, float *humidity, float *pressure)
{
    uint8_t data[8];
    bme280_read(0xF7, data, 8);

    int32_t adc_P = (data[0]<<12) | (data[1]<<4) | (data[2]>>4);
    int32_t adc_T = (data[3]<<12) | (data[4]<<4) | (data[5]>>4);
    int32_t adc_H = (data[6]<<8)  |  data[7];

    *temp = adc_T / 100.0f;
    *pressure = adc_P / 100.0f;
    *humidity = adc_H / 1024.0f;
}
