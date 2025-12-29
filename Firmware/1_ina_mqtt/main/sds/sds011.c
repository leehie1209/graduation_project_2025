#include "sds011.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board/board.h"

static const char *TAG = "SDS011";

// -------- Global data --------
sds011_data_t g_sds_data = {
    .pm25 = 0,
    .pm10 = 0,
    .valid = false,
};

// -------- UART CONFIG --------
#define SDS_UART_BUF_SIZE  256

bool sds011_read(float *pm25, float *pm10)
{
    uint8_t buf[10];

    int len = uart_read_bytes(SDS_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(1500));
    if (len < 10) {
        ESP_LOGW(TAG, "Received too short packet (%d bytes)", len);
        return false;
    }

    // Kiểm tra header/frame SDS011
    if (buf[0] != 0xAA || buf[1] != 0xC0) {
        ESP_LOGW(TAG, "Bad frame header");
        return false;
    }

    uint16_t pm25_raw = buf[2] | (buf[3] << 8);
    uint16_t pm10_raw = buf[4] | (buf[5] << 8);

    *pm25 = pm25_raw / 10.0f;
    *pm10 = pm10_raw / 10.0f;

    ESP_LOGI(TAG, "PM2.5=%.1f PM10=%.1f", *pm25, *pm10);
    return true;
}

// ---------------------------------------------------------
//       TASK ĐỌC SDS011 – CẬP NHẬT GIÁ TRỊ TOÀN CỤC
// ---------------------------------------------------------
static void sds011_task(void *arg)
{
    while (1)
    {
        float pm25 = 0, pm10 = 0;

        bool ok = sds011_read(&pm25, &pm10);

        if (ok) {
            g_sds_data.pm25 = pm25;
            g_sds_data.pm10 = pm10;
            g_sds_data.valid = true;
            ESP_LOGI(TAG, "Updated g_sds_data (PM2.5=%.1f, PM10=%.1f)", pm25, pm10);
        } else {
            ESP_LOGW(TAG, "SDS011 read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // 10s
    }
}

// ---------------------------------------------------------
//               KHỞI TẠO & START TASK
// ---------------------------------------------------------
void sds011_start_task(void)
{
    // Init UART
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(SDS_UART_NUM, &uart_config);
    uart_set_pin(SDS_UART_NUM, SDS_UART_TX_PIN, SDS_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(SDS_UART_NUM, SDS_UART_BUF_SIZE, 0, 0, NULL, 0);

    xTaskCreate(sds011_task, "sds011_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "SDS011 task started");
}
