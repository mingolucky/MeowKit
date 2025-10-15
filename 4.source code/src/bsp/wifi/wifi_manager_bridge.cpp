#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h> // tzapu/WiFiManager
#include "wifi_manager_bridge.h"

// LVGL 交互
extern "C" {
#include "ui/ui.h"
#include "lvgl.h"
}

static TaskHandle_t s_portal_task = nullptr;
static String s_last_ssid;
static String s_last_pass;
static lv_obj_t* s_wifi_popup = nullptr;
static bool s_popup_closed_by_user = false;
static bool s_refreshing_popup = false;

// Close handler: track user-close to avoid recreating popup
static void wifi_popup_event_cb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_DELETE) {
    s_wifi_popup = nullptr;
    if (s_refreshing_popup) {
      // deletion triggered by refresh; keep popup allowed
      s_refreshing_popup = false;
    } else {
      // user closed popup
      s_popup_closed_by_user = true;
      if (ui_Control_Center) {
        lv_scr_load(ui_Control_Center);
      }
    }
  }
}

static String get_device_suffix() {
  // Use the last two bytes of the chip MAC as the suffix, such as E985
  uint64_t mac = ESP.getEfuseMac();
  uint16_t suffix = (uint16_t)(mac & 0xFFFF);
  char buf[8];
  snprintf(buf, sizeof(buf), "%04X", suffix);
  return String(buf);
}

static void update_popup_text_async(const char* text) {
  // Copy text to heap, ensure lifetime until UI thread consumes it
  size_t n = strlen(text) + 1;
  char* heap_msg = (char*)malloc(n);
  if (!heap_msg) return;
  memcpy(heap_msg, text, n);
  lv_async_call(
    [](void* p){
      char* msg = (char*)p;
      if (s_popup_closed_by_user) {
        // user closed; don't show again
        free(msg);
        return;
      }
      // Recreate the popup for new text to avoid using lv_msgbox_set_text (may be unavailable)
      if (s_wifi_popup) {
        s_refreshing_popup = true;
        lv_obj_del(s_wifi_popup);
        s_wifi_popup = nullptr;
      }
  s_wifi_popup = lv_msgbox_create(lv_scr_act(), "WiFi Setup", msg, NULL, true);
      lv_obj_set_width(s_wifi_popup, 220);
      lv_obj_set_height(s_wifi_popup, 120);
      lv_obj_center(s_wifi_popup);
      lv_obj_add_event_cb(s_wifi_popup, wifi_popup_event_cb, LV_EVENT_DELETE, NULL);
      free(msg);
    },
    heap_msg
  );
}

static void portal_task(void* arg) {
  WiFi.mode(WIFI_STA); // Set it to STA first, WiFiManager will switch to AP internally
  //Ensure credential persistence and automatic reconnection
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFiManager wm;

  // Construct AP name: MeowKit-XXXX
  String apName = String("MeowKit-") + get_device_suffix();
  wm.setEnableConfigPortal(true);
  wm.setConfigPortalTimeout(180); // 3 minutes timeout
  wm.setShowStaticFields(true);
  wm.setShowInfoUpdate(true);

  // Show portal tip
  String tip = String("AP mode ") + apName + "\nOpen 192.168.4.1";
  update_popup_text_async(tip.c_str());

  bool res = wm.autoConnect(apName.c_str()); // Block until connected or timed out
  if (res) {
    s_last_ssid = WiFi.SSID();
    s_last_pass = WiFi.psk();
  String ok = String("Connected\nSSID: ") + s_last_ssid.c_str();
    update_popup_text_async(ok.c_str());
  } else {
  update_popup_text_async("Setup failed or timeout");
  }

  s_portal_task = nullptr;
  vTaskDelete(NULL);
}

void wifi_start_portal_async(void) {
  if (s_portal_task) return; // Already running
  s_popup_closed_by_user = false; // new session can show popup again
  xTaskCreatePinnedToCore(portal_task, "wm_portal", 12288, nullptr, 1, &s_portal_task, APP_CPU_NUM);
}

bool wifi_get_saved_credentials(char* ssid_out, size_t ssid_len, char* pass_out, size_t pass_len) {
  // If WiFi is connected, return directly
  if (WiFi.isConnected()) {
    strlcpy(ssid_out, WiFi.SSID().c_str(), ssid_len);
    strlcpy(pass_out, WiFi.psk().c_str(), pass_len);
    return true;
  }
  // Trying to read the last credentials from the system save (ESP32 NVS)
  WiFi.mode(WIFI_STA); // STA mode is required to get SSID/PSK
  String savedSsid = WiFi.SSID();
  String savedPsk  = WiFi.psk();
  if (savedSsid.length() > 0) {
    strlcpy(ssid_out, savedSsid.c_str(), ssid_len);
    strlcpy(pass_out, savedPsk.c_str(), pass_len);
    return true;
  }
  // If not connected, returns the most recent record passed by WiFiManager (which is persisted in NVS)
  if (!s_last_ssid.isEmpty()) {
    strlcpy(ssid_out, s_last_ssid.c_str(), ssid_len);
    strlcpy(pass_out, s_last_pass.c_str(), pass_len);
    return true;
  }
  // You can also try to read it from Preferences/NVS (WiFiManager has written it to the system storage, usually WiFi.begin will use it automatically)
  return false;
}
