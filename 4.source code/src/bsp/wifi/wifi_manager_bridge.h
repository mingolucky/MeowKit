#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Start the AP and network configuration (executed in an independent task, without blocking LVGL).
void wifi_start_portal_async(void);

// Get the saved SSID and password (read from NVS, or directly return WiFi.SSID/PSK if using WiFiManager built-in save).
// Returns whether it is available (connected or stored).
bool wifi_get_saved_credentials(char* ssid_out, size_t ssid_len, char* pass_out, size_t pass_len);

#ifdef __cplusplus
}
#endif
