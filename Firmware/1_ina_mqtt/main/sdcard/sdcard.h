#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sdcard_init(void);

esp_err_t sdcard_append_line(const char *path, const char *line);

esp_err_t sdcard_append_fmt(const char *path, const char *fmt, ...);

bool sdcard_is_ready(void);

#ifdef __cplusplus
}
#endif
