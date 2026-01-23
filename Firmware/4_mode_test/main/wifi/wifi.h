// wifi.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#define WIFI_SSID "Leehie"
#define WIFI_PASS "0987654321"
/**
 * @brief Khởi tạo WiFi ở chế độ STA và tự động kết nối
 */
void wifi_init_sta(void);

/**
 * @brief Ngắt kết nối WiFi (dùng khi sleep)
 */
void wifi_disconnect(void);

#ifdef __cplusplus
}
#endif
