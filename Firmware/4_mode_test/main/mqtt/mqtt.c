#include "mqtt_client.h"
#include "esp_log.h"
#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client;

/* ===== MQTT STATE ===== */
#define MQTT_CONNECTED_BIT BIT0
#define MQTT_FAIL_BIT      BIT1

static EventGroupHandle_t mqtt_event_group;
static int mqtt_retry = 0;
#define MQTT_MAX_RETRY 10

static void mqtt_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        mqtt_retry = 0;
        xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");

        if (mqtt_retry < MQTT_MAX_RETRY) {
            mqtt_retry++;
            ESP_LOGW(TAG, "Retry MQTT (%d/%d)", mqtt_retry, MQTT_MAX_RETRY);
            esp_mqtt_client_reconnect(client);
        } else {
            ESP_LOGE(TAG, "MQTT retry limit reached");
            xEventGroupSetBits(mqtt_event_group, MQTT_FAIL_BIT);
        }
        break;

    case MQTT_EVENT_DATA: {
        char data[event->data_len + 1];
        memcpy(data, event->data, event->data_len);
        data[event->data_len] = 0;

        ESP_LOGI(TAG, "MQTT RX: %s", data);

        if (strstr(data, "\"mode\":\"auto\"")) {
            system_state_set_mode(MODE_AUTO);
        }

        if (strstr(data, "\"mode\":\"remote\"")) {
            ESP_LOGI(TAG, "remote");
            system_state_set_mode(MODE_REMOTE);
        }

        if (strstr(data, "\"state\":1")) {
            system_state_set_remote_state(STATE_1);
        } else if (strstr(data, "\"state\":2")) {
            system_state_set_remote_state(STATE_2);
        } else if (strstr(data, "\"state\":3")) {
            system_state_set_remote_state(STATE_3);
        }
        break;
    }

    default:
        break;
    }
}

void mqtt_start(void)
{
    mqtt_event_group = xEventGroupCreate();

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = "mqtt://mqtt.tyckr.io:1883/",
        .credentials.client_id = "esp32_node01_9F3A",
        .network.timeout_ms = 5000,
    };


    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(
        client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL
    );

    esp_mqtt_client_start(client);

    ESP_LOGI(TAG, "Waiting MQTT broker...");

    /* ⏳ CHỜ MQTT CONNECT hoặc FAIL */
    EventBits_t bits = xEventGroupWaitBits(
        mqtt_event_group,
        MQTT_CONNECTED_BIT | MQTT_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & MQTT_CONNECTED_BIT) {
        ESP_LOGI(TAG, "MQTT ready, subscribing...");
        esp_mqtt_client_subscribe(
            client, "device/node01/control", 0
        );
    } else if (bits & MQTT_FAIL_BIT) {
        ESP_LOGE(TAG, "MQTT start aborted");
        esp_mqtt_client_stop(client);
    }
}
