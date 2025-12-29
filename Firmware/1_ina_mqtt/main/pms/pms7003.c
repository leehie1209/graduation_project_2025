#include "pms7003.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "board/board.h"

static const char *TAG = "PMS7003";

// -------- Global data --------
pms7003_data_t g_pms_data = {
    .pm1_0 = 0,
    .pm2_5 = 0,
    .pm10  = 0,
    .valid = false,
};

// -------- UART CONFIG --------
#define PMS_UART_BUF_SIZE  256

// ---------------------------------------------------------
//      ĐỌC FRAME PMS7003 (32 BYTES)
// ---------------------------------------------------------
static bool pms7003_read(uint16_t *pm1, uint16_t *pm25, uint16_t *pm10)
{
    uint8_t buf[32];

    int len = uart_read_bytes(PMS_UART_NUM, buf, sizeof(buf), pdMS_TO_TICKS(1500));
    if (len < 32) {
        ESP_LOGW(TAG, "Short frame (%d bytes)", len);
        return false;
    }

    // Header 0x42 0x4D
    if (buf[0] != 0x42 || buf[1] != 0x4D) {
        ESP_LOGW(TAG, "Invalid header");
        return false;
    }

    // Dữ liệu CF=1 (standard)
    *pm1  = (buf[10] << 8) | buf[11];
    *pm25 = (buf[12] << 8) | buf[13];
    *pm10 = (buf[14] << 8) | buf[15];

    ESP_LOGI(TAG, "PM1.0=%u PM2.5=%u PM10=%u", *pm1, *pm25, *pm10);
    return true;
}

// ---------------------------------------------------------
//           TASK ĐỌC PMS7003
// ---------------------------------------------------------
static void pms7003_task(void *arg)
{
    while (1)
    {
        uint16_t pm1 = 0, pm25 = 0, pm10 = 0;

        bool ok = pms7003_read(&pm1, &pm25, &pm10);

        if (ok) {
            g_pms_data.pm1_0 = pm1;
            g_pms_data.pm2_5 = pm25;
            g_pms_data.pm10  = pm10;
            g_pms_data.valid = true;

            ESP_LOGI(TAG, "Updated g_pms_data");
        } else {
            ESP_LOGW(TAG, "PMS7003 read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // 10s
    }
}

// ---------------------------------------------------------
//           INIT UART + START TASK
// ---------------------------------------------------------
void pms7003_start_task(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(PMS_UART_NUM, &uart_config);
    uart_set_pin(PMS_UART_NUM,
                 PMS_UART_TX_PIN,
                 PMS_UART_RX_PIN,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);

    uart_driver_install(PMS_UART_NUM, PMS_UART_BUF_SIZE, 0, 0, NULL, 0);

    xTaskCreate(pms7003_task, "pms7003_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "PMS7003 task started");
}
