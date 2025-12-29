#ifndef MQTT_H_
#define MQTT_H_

#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include <inttypes.h> // để dùng PRIi32

void mqtt_app_start(void);
extern esp_mqtt_client_handle_t global_mqtt_client;


#endif
