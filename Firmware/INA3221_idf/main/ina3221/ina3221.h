#ifndef INA3221_H
#define INA3221_H

#include <stdint.h>
#include "driver/i2c.h"

typedef enum {
    INA3221_CH1 = 0,
    INA3221_CH2,
    INA3221_CH3,
    INA3221_CH_NUM
} ina3221_channel_t;

typedef struct {
    i2c_port_t i2c_port;
    uint8_t i2c_addr;

    float shunt_res[INA3221_CH_NUM];
} ina3221_dev_t;

// ==== Init ====
void ina3221_init(ina3221_dev_t *dev, i2c_port_t port, uint8_t addr);

// ==== Config ====
void ina3221_set_shunt_res(ina3221_dev_t *dev, ina3221_channel_t ch, float res_ohm);

// ==== Read registers ====
uint16_t ina3221_get_reg(ina3221_dev_t *dev, uint8_t reg);

// ==== Measurements ====
float ina3221_get_shunt_voltage(ina3221_dev_t *dev, ina3221_channel_t ch);
float ina3221_get_current(ina3221_dev_t *dev, ina3221_channel_t ch);
float ina3221_get_bus_voltage(ina3221_dev_t *dev, ina3221_channel_t ch);
float ina3221_get_power(ina3221_dev_t *dev, ina3221_channel_t ch)

#endif
