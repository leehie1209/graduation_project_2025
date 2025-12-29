// #ifndef BOARD_H
#define BOARD_H

#include "driver/uart.h"
#include "driver/i2c.h"

/* =========================================================
 *                  SDS011 – UART
 * ========================================================= */
#define SDS_UART_NUM       UART_NUM_1
#define SDS_UART_TX_PIN    33
#define SDS_UART_RX_PIN    32

/* =========================================================
 *                  PMS7003 – UART
 * ========================================================= */
#define PMS_UART_NUM       UART_NUM_2
#define PMS_UART_TX_PIN    17
#define PMS_UART_RX_PIN    16

/* =========================================================
 *                  BME280 – I2C
 * ========================================================= */
#define BME_I2C_NUM        I2C_NUM_0
#define BME_I2C_SDA_PIN    21
#define BME_I2C_SCL_PIN    22

/* =========================================================
 *                  RTC – I2C (nếu dùng)
 * ========================================================= */
#define RTC_I2C_NUM        I2C_NUM_1
#define RTC_I2C_SDA_PIN    26
#define RTC_I2C_SCL_PIN    27

/* =========================================================
 *                  SD CARD – SPI
 * ========================================================= */
#define SD_CS_PIN          5
#define SD_MOSI_PIN        23
#define SD_MISO_PIN        19
#define SD_SCK_PIN         18
