#include "esp_log.h"
#include "mqtt/mqtt.h"
#include "board.h"

static const char *TAG = "MQTT";

// 👉 BIẾN GLOBAL MQTT — PHẢI CÓ Ở ĐÂY
esp_mqtt_client_handle_t global_mqtt_client = NULL;

static void mqtt_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");

            esp_mqtt_client_publish(client, "data/value", "hello", 0, 1, 0);

            int msg_id = esp_mqtt_client_subscribe(client, "data/value/Gate", 1);
            ESP_LOGI(TAG, "Subscribed to data/value/Gate_1, msg_id=%d", msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT DATA:");
            printf("TOPIC=%.*s\n", event->topic_len, event->topic);
            printf("DATA=%.*s\n", event->data_len, event->data);
            break;

        default:
            ESP_LOGI(TAG, "Event id: %" PRIi32, event_id);
            break;
    }
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = "mqtt://mqtt.tyckr.io:1883/",
        .credentials.client_id = "esp32_test",
    };

    // 👉 GÁN CHO GLOBAL LUÔN
    global_mqtt_client = esp_mqtt_client_init(&cfg);

    esp_mqtt_client_register_event(global_mqtt_client,
                                   ESP_EVENT_ANY_ID,
                                   mqtt_event_handler,
                                   NULL);

    esp_mqtt_client_start(global_mqtt_client);
}
