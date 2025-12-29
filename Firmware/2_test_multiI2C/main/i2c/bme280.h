#ifndef BME280_H
#define BME280_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BME280_ADDR  0x76

void bme280_init(void);
void bme280_read_data(float *temp, float *humidity, float *pressure);

#ifdef __cplusplus
}
#endif

#endif
