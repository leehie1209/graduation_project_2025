#include "i2c/ina226.h"
#include "i2c/i2c_master.h"
#include "esp_log.h"
static const char *TAG = "INA226";

#define INA226_ADDR  0x40

esp_err_t ina226_write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = {reg, value >> 8, value & 0xFF};
    return i2c_master_write_to_device(I2C_MASTER_PORT, INA226_ADDR, buf, 3, 1000 / portTICK_PERIOD_MS);
}

esp_err_t ina226_read_reg(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_PORT, INA226_ADDR,
                                                 &reg, 1, data, 2, 1000 / portTICK_PERIOD_MS);
    if (ret == ESP_OK)
        *value = (data[0] << 8) | data[1];
    return ret;
}

void ina226_init()
{
    // Config: averages=1, conversion=1100us, mode=shunt+bus continuous
    ina226_write_reg(0x00, 0x4527);

    // Calibration (ví dụ dòng tối đa 5A)
    ina226_write_reg(0x05, 1024);
}

void ina226_read(float *bus_voltage, float *current)
{
    uint16_t raw_bus, raw_current;

    ina226_read_reg(0x02, &raw_bus);
    *bus_voltage = raw_bus * 1.25f / 1000.0f; // 1.25mV/LSB

    ina226_read_reg(0x04, &raw_current);
    *current = (int16_t) raw_current * 0.001f; // scale tùy calibration
}
