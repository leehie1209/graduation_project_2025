#ifndef INA226_H
#define INA226_H

#include "stdint.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INA226_ADDR  0x40

/**
 * @brief Ghi thanh ghi INA226
 */
esp_err_t ina226_write_reg(uint8_t reg, uint16_t value);

/**
 * @brief Đọc thanh ghi INA226
 */
esp_err_t ina226_read_reg(uint8_t reg, uint16_t *value);

/**
 * @brief Khởi tạo cảm biến INA226
 */
void ina226_init(void);

/**
 * @brief Đọc điện áp & dòng từ INA226
 *
 * @param bus_voltage Điện áp (V)
 * @param current     Dòng (A)
 */
void ina226_read(float *bus_voltage, float *current);

#ifdef __cplusplus
}
#endif

#endif // INA226_H
