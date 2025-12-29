#include "sdcard/sdcard.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"

#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

/* ====== BOARD CONFIG ====== */
#include "board/board.h"

static const char *TAG = "SDCARD";

/* ====== CONFIG ====== */
#define SD_MOUNT_POINT   "/sdcard"
#define SD_MAX_FILES     4

/* ====== STATIC ====== */
static bool s_ready = false;
static SemaphoreHandle_t s_mutex;
static sdmmc_card_t *s_card;

/* ====== API ====== */

esp_err_t sdcard_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
        ESP_LOGE(TAG, "Create mutex failed");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret;

    /* SPI host từ board.h */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = HSPI_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_CS_PIN;
    slot_cfg.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = SD_MAX_FILES,
        .allocation_unit_size = 16 * 1024,
    };

    ESP_LOGI(TAG, "Mounting SD card...");
    ret = esp_vfs_fat_sdspi_mount(
        SD_MOUNT_POINT,
        &host,
        &slot_cfg,
        &mount_cfg,
        &s_card
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sdmmc_card_print_info(stdout, s_card);

    s_ready = true;
    ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);
    return ESP_OK;
}

bool sdcard_is_ready(void)
{
    return s_ready;
}

esp_err_t sdcard_append_line(const char *path, const char *line)
{
    if (!s_ready || !path || !line) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    FILE *f = fopen(path, "a");
    if (!f) {
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "Open file failed: %s", path);
        return ESP_FAIL;
    }

    fprintf(f, "%s\n", line);
    fclose(f);

    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t sdcard_append_fmt(const char *path, const char *fmt, ...)
{
    if (!s_ready || !path || !fmt) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    FILE *f = fopen(path, "a");
    if (!f) {
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "Open file failed: %s", path);
        return ESP_FAIL;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);

    fprintf(f, "\n");
    fclose(f);

    xSemaphoreGive(s_mutex);
    return ESP_OK;
}
