#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include "driver/i2c.h"
#include "esp_err.h"

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_PORT             I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi tạo I2C Master
 */
esp_err_t i2c_master_init(void);

#ifdef __cplusplus
}
#endif

#endif // I2C_MASTER_H
