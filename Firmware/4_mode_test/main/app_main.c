#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"

#include "system_state.h"
#include "wifi.h"
#include "mqtt.h"

static const char *TAG = "MAIN";

#define LED_PIN GPIO_NUM_5

void led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(LED_PIN, 0); // LED OFF ban đầu
}

/* Các hàm bạn ĐÃ có */
bool is_daytime_detect(void)
{
    // ví dụ: đọc ADC, LDR
    return true;
}

int battery_get_percent(void)
{
    return 75;
}

static void blink(int time_ms)
{
    while (1)
    {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(time_ms));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(time_ms));
    }
}

/* ==== HÀM CHUYỂN MODE ==== */

static void enter_normal_mode(void)
{
    ESP_LOGI(TAG, "Enter NORMAL mode");
    blink(1000);
    // Bật WiFi, sensor, task đo, MQTT...
}

static void enter_light_sleep_mode(void)
{

    ESP_LOGI(TAG, "Enter LIGHT SLEEP mode");
    blink(500);
    // esp_sleep_enable_timer_wakeup(30 * 60 * 1000000ULL);
    // esp_light_sleep_start();
}

/* ==== TASK CHÍNH ==== */

void app_main(void)
{
    wifi_init_sta();
    mqtt_start();

    system_state_init();

    while (1) {

        bool is_day = is_daytime_detect();
        float batt  = battery_get_percent();

        system_state_process(is_day, batt);

        /* Thực thi theo STATE */
        switch (system_state_get_current()) {

        case STATE_1:
            enter_normal_mode();
            break;

        case STATE_2:
            enter_normal_mode();   // nhưng giảm tần suất đo
            break;

        case STATE_3:
            enter_light_sleep_mode();
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
