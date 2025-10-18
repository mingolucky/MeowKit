#include "ui.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <iostream>
#include "config.h"
#include "ui_keyboard.h"
#include <string.h>

// Simple container to pass WiFi scan items to LVGL async callbacks.
struct WifiScanItem
{
    int idx;
    char *ssid;
};

static lv_obj_t *file_list = nullptr;
lv_obj_t *ui_WiFi_input_password = nullptr;
static TaskHandle_t s_wifi_task = nullptr;
static TaskHandle_t s_btn_a_task = nullptr;

void ui_WiFi_keyboard_screen_init(void)
{
    ui_WiFi_input_password = lv_obj_create(nullptr);
    lv_obj_clear_flag(ui_WiFi_input_password, LV_OBJ_FLAG_SCROLLABLE); /// Flags
    create_3x4_keyboard(ui_WiFi_input_password);
}

// The callback for switching to Home in the LVGL main thread (called by lv_async_call)
static void btn_a_press_lv_cb(void *ud)
{
    int which = (int)(intptr_t)ud;
    if (lv_scr_act() != ui_WiFi) return;

    lv_indev_wait_release(lv_indev_get_act());

    if (which == 1) // A
    {
        extern lv_obj_t *ui_Home;
        extern void ui_Home_screen_init(void);
        _ui_screen_change(&ui_Home, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_Home_screen_init);
    }
    else if (which == 2) // B
    {
        extern lv_obj_t *ui_Control_Center;
        extern void ui_Control_Center_screen_init(void);
        _ui_screen_change(&ui_Control_Center, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_Control_Center_screen_init);
    }

    // task done cleanup if needed
    s_wifi_task = nullptr;
}

// Background task: poll physical button A, detect press (debounce), trigger return to Home when the current button is ui_WiFi
static void btn_a_monitor_task(void *arg)
{
    const TickType_t sampleDelay = pdMS_TO_TICKS(50);
    bool lastA = false;
    bool lastB = false;
    while (true)
    {
        bool pressedA = (digitalRead(5) == LOW); // A active low
        bool pressedB = (digitalRead(4) == LOW); // B active low

        if (pressedA && !lastA)
        {
            vTaskDelay(pdMS_TO_TICKS(20)); // debounce
            if (digitalRead(5) == LOW)
            {
                lv_async_call(btn_a_press_lv_cb, (void *)(intptr_t)1); // pass 1 => A
            }
        }

        if (pressedB && !lastB)
        {
            vTaskDelay(pdMS_TO_TICKS(20)); // debounce
            if (digitalRead(4) == LOW)
            {
                lv_async_call(btn_a_press_lv_cb, (void *)(intptr_t)2); // pass 2 => B
            }
        }

        lastA = pressedA;
        lastB = pressedB;
        vTaskDelay(sampleDelay);
    }
}

static void sd_folder_enter_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        int idx = (int)(intptr_t)lv_event_get_user_data(e);
        std::cout << "Clicked on WiFi network index: " << idx << std::endl;
        wifi_ssid = String(lv_list_get_btn_text(file_list, lv_event_get_target(e)));
        _ui_screen_change(&ui_WiFi_input_password, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_WiFi_keyboard_screen_init);
    }
}

static void add_wifi_button_async(void *ud)
{
    WifiScanItem *it = (WifiScanItem *)ud;
    if (file_list && it && it->ssid)
    {
        lv_obj_t *btn = lv_list_add_btn(file_list, LV_SYMBOL_WIFI, it->ssid);
        lv_obj_add_event_cb(btn, sd_folder_enter_event, LV_EVENT_CLICKED, (void *)(intptr_t)it->idx);
    }
    if (it)
    {
        if (it->ssid)
            free(it->ssid);
        free(it);
    }
}

// Read saved SSID/password from NVS
static bool loadWiFiCredentials(String &out_ssid, String &out_pass)
{
    prefs.begin(PREF_NAMESPACE, false);
    out_ssid = prefs.getString(KEY_SSID, "");
    out_pass = prefs.getString(KEY_PASS, "");
    prefs.end();
    return out_ssid.length() > 0;
}

// If there are saved credentials, try to connect automatically (return whether the connection is successful)
static bool autoConnectSavedWiFi(int timeoutMs = 5000)
{
    String ssid, pass;
    if (!loadWiFiCredentials(ssid, pass))
        return false;

    wifi_ssid = ssid; 
    std::cout << "Auto connect: " << ssid.c_str() << std::endl;

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int waited = 0;
    const int step = 200;
    while (WiFi.status() != WL_CONNECTED && waited < timeoutMs)
    {
        delay(step);
        waited += step;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        std::cout << "Auto-connected to " << ssid.c_str() << std::endl;
        return true;
    }
    else
    {
        std::cout << "Auto-connect failed for " << ssid.c_str() << std::endl;
        // Optional: Clear incorrect saved credentials prefs.putString(KEY_SSID,""); prefs.putString(KEY_PASS,"");
        return false;
    }
}

static void searchWifi_task(void *arg)
{
    // Ensure the list is cleared before filling
    if (file_list)
    {
        // schedule clear on main thread to be safe
        lv_async_call([](void *)
                      { if (file_list) lv_obj_clean(file_list); }, nullptr);
    }

    int networkCount = WiFi.scanNetworks(); // blocking but in background task now
    if (networkCount <= 0)
    {
        std::cout << "No WiFi networks found." << std::endl;
    }
    else
    {
        std::cout << networkCount << " WiFi networks found." << std::endl;
        for (int i = 0; i < networkCount; ++i)
        {
            String ss = WiFi.SSID(i);
            WifiScanItem *it = (WifiScanItem *)malloc(sizeof(WifiScanItem));
            if (!it)
                continue;
            it->idx = i;
            it->ssid = strdup(ss.c_str());
            // schedule button creation on LVGL main thread
            lv_async_call(add_wifi_button_async, it);
            vTaskDelay(pdMS_TO_TICKS(10)); // small delay to yield
        }
        // Try to automatically connect to saved WiFi
        if (autoConnectSavedWiFi())
        {
            obj_status = lv_label_create(ui_WiFi);
            lv_obj_set_style_text_color(obj_status, lv_color_hex(0x00FF00), 0);
            lv_label_set_text(obj_status, "auto connected to saved WiFi");
            lv_obj_align(obj_status, LV_ALIGN_CENTER, 0, 0);
            lv_timer_create(label_hide_timer_cb, 3000, obj_status);
        }
        else
        {
            std::cout<<" WiFi: no saved cred or connect failed\n";
        }
    }
    // IMPORTANT: free scan results so next scan works
    WiFi.scanDelete();
    // task done, delete self
    s_wifi_task = nullptr;
    vTaskDelete(NULL);
}
static void event_wifi_switch(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        if (lv_obj_has_state(ui_wifi_switch, LV_STATE_CHECKED))
        {
            // clear old list immediately (on main thread)
            if (file_list)
                lv_obj_clean(file_list);

            // don't start multiple concurrent tasks
            if (s_wifi_task == nullptr)
            {
#ifdef CONFIG_FREERTOS_UNICORE
                xTaskCreate(
                    searchWifi_task, "wm_wifi", 8192, NULL, 1, &s_wifi_task);
#else
                // Pin to APP_CPU_NUM if defined (ESP32); fallback to xTaskCreate if not available
#ifdef APP_CPU_NUM
                xTaskCreatePinnedToCore(searchWifi_task, "wm_wifi", 8192, NULL, 1, &s_wifi_task, APP_CPU_NUM);
#else
                xTaskCreate(searchWifi_task, "wm_wifi", 8192, NULL, 1, &s_wifi_task);
#endif
#endif
            }
            else
            {
                std::cout << "WiFi scan already running" << std::endl;
            }
        }
        else
        {
            if (file_list)
            {
                lv_obj_clean(file_list);
            }
        }
    }
}

void ui_WiFi_screen_init(void)
{
    ui_WiFi = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_WiFi, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    ui_normal_wifi_bg = lv_img_create(ui_WiFi);
    lv_img_set_src(ui_normal_wifi_bg, &ui_img_ui_normal_bg_png);
    lv_obj_set_width(ui_normal_wifi_bg, LV_SIZE_CONTENT);  /// 320
    lv_obj_set_height(ui_normal_wifi_bg, LV_SIZE_CONTENT); /// 240
    lv_obj_set_align(ui_normal_wifi_bg, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_normal_wifi_bg, LV_OBJ_FLAG_ADV_HITTEST);  /// Flags
    lv_obj_clear_flag(ui_normal_wifi_bg, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    ui_normal_wifi_list = lv_img_create(ui_WiFi);
    lv_img_set_src(ui_normal_wifi_list, &ui_img_ui_normal_list_png);
    lv_obj_set_width(ui_normal_wifi_list, LV_SIZE_CONTENT);  /// 302
    lv_obj_set_height(ui_normal_wifi_list, LV_SIZE_CONTENT); /// 231
    lv_obj_set_align(ui_normal_wifi_list, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_normal_wifi_list, LV_OBJ_FLAG_ADV_HITTEST);  /// Flags
    lv_obj_clear_flag(ui_normal_wifi_list, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    ui_wifi_list = lv_label_create(ui_WiFi);
    lv_obj_set_width(ui_wifi_list, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(ui_wifi_list, LV_SIZE_CONTENT); /// 1
    lv_obj_set_x(ui_wifi_list, -105);
    lv_obj_set_y(ui_wifi_list, -100);
    lv_obj_set_align(ui_wifi_list, LV_ALIGN_CENTER);
    lv_label_set_text(ui_wifi_list, "wifi list");
    lv_obj_set_style_text_color(ui_wifi_list, lv_color_hex(0xA3DE00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_wifi_list, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_wifi_list, &ui_font_name, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_wifi_a_btn_home = lv_label_create(ui_WiFi);
    lv_obj_set_width(ui_wifi_a_btn_home, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(ui_wifi_a_btn_home, LV_SIZE_CONTENT); /// 1
    lv_obj_set_x(ui_wifi_a_btn_home, 0);
    lv_obj_set_y(ui_wifi_a_btn_home, 99);
    lv_obj_set_align(ui_wifi_a_btn_home, LV_ALIGN_CENTER);
    lv_label_set_text(ui_wifi_a_btn_home, "->control center");
    lv_obj_set_style_text_color(ui_wifi_a_btn_home, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_wifi_a_btn_home, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_wifi_a_btn_home, &ui_font_name, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_wifi_b_btn_back = lv_label_create(ui_WiFi);
    lv_obj_set_width(ui_wifi_b_btn_back, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(ui_wifi_b_btn_back, LV_SIZE_CONTENT); /// 1
    lv_obj_set_x(ui_wifi_b_btn_back, 112);
    lv_obj_set_y(ui_wifi_b_btn_back, 99);
    lv_obj_set_align(ui_wifi_b_btn_back, LV_ALIGN_CENTER);
    lv_label_set_text(ui_wifi_b_btn_back, "->back");
    lv_obj_set_style_text_color(ui_wifi_b_btn_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_wifi_b_btn_back, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_wifi_b_btn_back, &ui_font_name, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_btn_a_home_wifi = lv_btn_create(ui_WiFi);
    lv_obj_set_width(ui_btn_a_home_wifi, 100);
    lv_obj_set_height(ui_btn_a_home_wifi, 30);
    lv_obj_set_x(ui_btn_a_home_wifi, -65);
    lv_obj_set_y(ui_btn_a_home_wifi, 100);
    lv_obj_set_align(ui_btn_a_home_wifi, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_btn_a_home_wifi, LV_OBJ_FLAG_SCROLL_ON_FOCUS); /// Flags
    lv_obj_clear_flag(ui_btn_a_home_wifi, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_style_radius(ui_btn_a_home_wifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btn_a_home_wifi, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btn_a_home_wifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_src(ui_btn_a_home_wifi, &ui_img_ui_normal_btn_a_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_btn_a_home_wifi, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_btn_a_home_wifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_btn_b_back_wifi = lv_btn_create(ui_WiFi);
    lv_obj_set_width(ui_btn_b_back_wifi, 100);
    lv_obj_set_height(ui_btn_b_back_wifi, 30);
    lv_obj_set_x(ui_btn_b_back_wifi, 70);
    lv_obj_set_y(ui_btn_b_back_wifi, 100);
    lv_obj_set_align(ui_btn_b_back_wifi, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_btn_b_back_wifi, LV_OBJ_FLAG_SCROLL_ON_FOCUS); /// Flags
    lv_obj_clear_flag(ui_btn_b_back_wifi, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_style_radius(ui_btn_b_back_wifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_btn_b_back_wifi, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_btn_b_back_wifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_src(ui_btn_b_back_wifi, &ui_img_ui_normal_btn_b_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_btn_b_back_wifi, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_btn_b_back_wifi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    file_list = lv_list_create(ui_WiFi);
    lv_obj_set_size(file_list, 300, 150);
    lv_obj_set_align(file_list, LV_ALIGN_TOP_MID);
    lv_obj_set_x(file_list, 0);
    lv_obj_set_y(file_list, 50);
    lv_obj_set_style_bg_opa(file_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(file_list, 0, 0);
    // Make the list receive keyboard focus
    lv_group_add_obj(lv_group_get_default(), file_list);
    lv_group_focus_obj(file_list);

    ui_wifi_switch = lv_switch_create(ui_WiFi);
    lv_obj_set_width(ui_wifi_switch, 40);
    lv_obj_set_height(ui_wifi_switch, 20);
    lv_obj_set_x(ui_wifi_switch, 110);
    lv_obj_set_y(ui_wifi_switch, -99);
    lv_obj_set_align(ui_wifi_switch, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_img_src(ui_wifi_switch, &ui_img_ui_wifi_off_png, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(ui_wifi_switch, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(ui_wifi_switch, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_wifi_switch, event_wifi_switch, LV_EVENT_VALUE_CHANGED, NULL);
    // lv_obj_add_event_cb(ui_btn_a_home_wifi, ui_event_btn_a_home_wifi, LV_EVENT_ALL, NULL);

    // Start the Button A monitoring task (if not started)
    if (s_btn_a_task == NULL)
    {
#ifdef CONFIG_FREERTOS_UNICORE
        xTaskCreate(btn_a_monitor_task, "btnA", 2048, NULL, 1, &s_btn_a_task);
#else
#ifdef APP_CPU_NUM
        xTaskCreatePinnedToCore(btn_a_monitor_task, "btnA", 2048, NULL, 1, &s_btn_a_task, APP_CPU_NUM);
#else
        xTaskCreate(btn_a_monitor_task, "btnA", 2048, NULL, 1, &s_btn_a_task);
#endif
#endif
    }
}
