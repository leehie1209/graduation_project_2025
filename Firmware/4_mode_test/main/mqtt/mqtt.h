// mqtt.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi động MQTT client và subscribe topic điều khiển
 */
void mqtt_start(void);

/**
 * @brief Dừng MQTT client
 */
void mqtt_stop(void);

#ifdef __cplusplus
}
#endif
