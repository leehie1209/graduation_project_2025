#pragma once

#include <stdbool.h>   // BẮT BUỘC cho bool

/* ===== MODE ĐIỀU KHIỂN ===== */
typedef enum {
    MODE_AUTO = 0,
    MODE_REMOTE
} control_mode_t;

/* ===== TRẠNG THÁI THIẾT BỊ ===== */
typedef enum {
    STATE_1 = 1,
    STATE_2,
    STATE_3
} device_state_t;

/* ===== API ===== */
void system_state_init(void);

void system_state_set_mode(control_mode_t mode);
void system_state_set_remote_state(device_state_t state);

void system_state_process(bool is_daytime,
                          float battery_percent);

device_state_t system_state_get_current(void);
