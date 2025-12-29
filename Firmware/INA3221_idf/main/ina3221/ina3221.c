#include "ina3221/ina3221.h"
// #include "driver/i2c.h"

// ==============================
// Low-level I2C Read/Write
// ==============================
static esp_err_t _read_word(ina3221_dev_t *dev, uint8_t reg, uint16_t *val)
{
    uint8_t data[2];

    esp_err_t ret = i2c_master_write_read_device(
        dev->i2c_port,
        dev->i2c_addr,
        &reg, 1,
        data, 2,
        50 / portTICK_PERIOD_MS
    );

    if (ret != ESP_OK) return ret;

    *val = (data[0] << 8) | data[1];
    return ESP_OK;
}

static esp_err_t _write_word(ina3221_dev_t *dev, uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = {reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)};

    return i2c_master_write_to_device(
        dev->i2c_port,
        dev->i2c_addr,
        buf, 3,
        50 / portTICK_PERIOD_MS
    );
}

// ==============================
// Public API
// ==============================
void ina3221_init(ina3221_dev_t *dev, i2c_port_t port, uint8_t addr)
{
    dev->i2c_port = port;
    dev->i2c_addr = addr;

    // default 0.1 ohm shunts
    dev->shunt_res[INA3221_CH1] = 0.1f;
    dev->shunt_res[INA3221_CH2] = 0.1f;
    dev->shunt_res[INA3221_CH3] = 0.1f;

    // config register: continuous mode, all channels on
    _write_word(dev, 0x00, 0x7127);
}

// ==============================
// Set custom shunt resistor
// ==============================
void ina3221_set_shunt_res(ina3221_dev_t *dev, ina3221_channel_t ch, float res_ohm)
{
    if (ch > INA3221_CH3) return;
    if (res_ohm > 0.0f)
        dev->shunt_res[ch] = res_ohm;
}

// ==============================
// Read register
// ==============================
uint16_t ina3221_get_reg(ina3221_dev_t *dev, uint8_t reg)
{
    uint16_t v = 0;
    _read_word(dev, reg, &v);
    return v;
}

// ==============================
// Measurements
// ==============================
float ina3221_get_shunt_voltage(ina3221_dev_t *dev, ina3221_channel_t ch)
{
    static const uint8_t reg_map[] = {0x01, 0x03, 0x05};

    uint16_t raw = ina3221_get_reg(dev, reg_map[ch]);
    int16_t val = (int16_t)raw;

    // LSB = 40µV
    return val * 40.0f;  // microvolt
}

float ina3221_get_current(ina3221_dev_t *dev, ina3221_channel_t ch)
{
    float v_uV = ina3221_get_shunt_voltage(dev, ch);
    float r = dev->shunt_res[ch];

    // I = V/R (convert uV → V)
    return (v_uV / 1e6f) / r;
}

float ina3221_get_bus_voltage(ina3221_dev_t *dev, ina3221_channel_t ch)
{
    static const uint8_t reg_map[] = {0x02, 0x04, 0x06};

    uint16_t raw = ina3221_get_reg(dev, reg_map[ch]);

    // LSB = 8mV
    return (raw >> 3) * 0.008f;
}

float ina_getPowerCompensated(ina3221_ch_t channel)
{
    float voltage = ina_getVoltage(channel);            
    float current = ina_getCurrentCompensated(channel); 
    return voltage * current;
}

// ==============================
// Power calculation
// ==============================
float ina3221_get_power(ina3221_dev_t *dev, ina3221_channel_t ch)
{
    float voltage = ina3221_get_bus_voltage(dev, ch); // V
    float current = ina3221_get_current(dev, ch);     // A

    return voltage * current; // W
}
