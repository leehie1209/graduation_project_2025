#include "system_state.h"

#include <stdbool.h>        // BẮT BUỘC
#include "esp_log.h"

static const char *TAG = "SYS_STATE";

/* ===== BIẾN NỘI BỘ ===== */
static control_mode_t current_mode  = MODE_AUTO;
static device_state_t current_state = STATE_1;
static device_state_t remote_state  = STATE_1;

/* ===== INIT ===== */
void system_state_init(void)
{
    current_mode  = MODE_AUTO;
    current_state = STATE_1;
}

/* ===== SET MODE ===== */
void system_state_set_mode(control_mode_t mode)
{
    current_mode = mode;
    ESP_LOGI(TAG, "Set mode: %s",
             mode == MODE_AUTO ? "AUTO" : "REMOTE");
}

/* ===== REMOTE STATE ===== */
void system_state_set_remote_state(device_state_t state)
{
    remote_state = state;
    ESP_LOGI(TAG, "Remote force state: %d", state);
}

/* ===== AUTO DECISION ===== */
static device_state_t auto_state_decision(bool is_daytime,
                                          float battery_percent)
{
    if (is_daytime) {
        if (battery_percent >= 90.0f)
            return STATE_1;
        else
            return STATE_2;
    } else {
        if (battery_percent > 50.0f)
            return STATE_2;
        else
            return STATE_3;
    }
}

/* ===== MAIN PROCESS ===== */
void system_state_process(bool is_daytime,
                          float battery_percent)
{
    if (current_mode == MODE_REMOTE) {
        current_state = remote_state;
    } else {
        current_state = auto_state_decision(
            is_daytime, battery_percent
        );
    }

    /* Chỉ NÊU YÊU CẦU – không xử lý phần cứng */
    switch (current_state) {

    case STATE_1:
        ESP_LOGI(TAG, "STATE 1: Full operation required");
        break;

    case STATE_2:
        ESP_LOGI(TAG, "STATE 2: Power saving operation required");
        break;

    case STATE_3:
        ESP_LOGI(TAG, "STATE 3: Low power / light sleep required");
        break;

    default:
        break;
    }
}

/* ===== GET CURRENT ===== */
device_state_t system_state_get_current(void)
{
    return current_state;
}
