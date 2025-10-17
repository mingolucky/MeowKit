#pragma once
#include <lvgl.h>
#include <string>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h> 
#include <Preferences.h>

String wifi_ssid = "";
lv_obj_t *obj_status = NULL;
static Preferences prefs;
static const char *PREF_NAMESPACE = "meowkit";
static const char *KEY_SSID = "ssid";
static const char *KEY_PASS = "pass";

// Define 3×4 keyboard character map (12 buttons)
static const char * btn_chars[15] = {
    "1abc",   // 0: 1, a, b, c
    "2def",   // 1: 2, d, e, f
    "3ghi",   // 2: 3, g, h, i
    "4jkl",   // 3: 4, j, k, l
    "5mno",   // 4: 5, m, n, o
    "6pqr",  // 5: 6, p, q, r, s
    "7stu",   // 6: 7, t, u, v
    "8vwx",  // 7: 8, w, x, y, z
    "9yz",  // 8: 9, ., ,, !, ?
    "0",      // 9: 0
    " DEL ",  // 10: delete
    " ,.!? ",  // 11: Common punctuation marks
    " EN ",     // 12: Confirm input completion
    " EN ",     // 13: Confirm input completion
    " EN "     // 14: Confirm input completion
};

// Button display text
static const char * btn_display_text[15] = {
    "1\nabc",
    "2\ndef", 
    "3\nghi",
    "4\njkl",
    "5\nmno",
    "6\npqr",
    "7\nstu",
    "8\nvwx",
    "9\nyz",
    "0",
    "DEL",
    ",.!?",
    "EN",
    "EN",
    "EN"
};

// Button State Structure
typedef struct 
{
    uint8_t click_count;    // Current click count
    const char *chars;      // All characters corresponding to this button
    uint8_t chars_len;      // Number of characters
    uint32_t last_click_time; // Last click time
} btn_state_t;

static btn_state_t btn_states[15];
static lv_obj_t * textarea;     // Text input area
static int8_t last_btn_id = -1; // Record the button ID of the last click

static void saveWiFiCredentials(const char* ssid, const char* pass) {
    prefs.begin(PREF_NAMESPACE, false);
    prefs.putString(KEY_SSID, ssid ? ssid : "");
    prefs.putString(KEY_PASS, pass ? pass : "");
    prefs.end();
}

// Initialize button state
void init_button_states(void) 
{
    for (int i = 0; i < 15; i++) 
    {
        btn_states[i].click_count = 0;
        btn_states[i].chars = btn_chars[i];
        btn_states[i].chars_len = strlen(btn_chars[i]);
        btn_states[i].last_click_time = 0;
    }
}

// Delete the last character
void delete_last_char(void) 
{
    lv_textarea_del_char(textarea);
}

// Add characters to the text area
void add_char_to_textarea(char c) 
{
    char str[2] = {c, '\0'};
    lv_textarea_add_text(textarea, str);
}

// Timer callback: delete the prompt label after 3 seconds
static void label_hide_timer_cb(lv_timer_t * timer)
{
    lv_obj_t * lbl = (lv_obj_t *)timer->user_data;
    if (lbl) lv_obj_del(lbl);
    obj_status = NULL;
    lv_timer_del(timer);
}

// Button click event handling
static void keyboard_btn_event_cb(lv_event_t * e) 
{
    lv_obj_t * btn = lv_event_get_target(e);
    uint32_t btn_id = (uint32_t)lv_obj_get_user_data(btn);
    uint32_t current_time = lv_tick_get();
    
    if (btn_id >= 13) return;
    
    if (btn_id == 10) 
    { 
        // Delete button
        delete_last_char();
        return;
    }
    if (btn_id == 12) 
    { 
        String pass = lv_textarea_get_text(textarea);
        String retMsg  = ""; 
        WiFi.disconnect(true);       
        WiFi.scanDelete();           // Release last scan results
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.persistent(false);      // Disable writing new credentials to flash (on demand)
        WiFi.mode(WIFI_STA);
#ifdef ESP32
        // Optional: Disable power saving to prevent handshake from being interrupted (requires including <esp_wifi.h>)
        esp_wifi_set_ps(WIFI_PS_NONE);
#endif
        WiFi.begin(wifi_ssid.c_str(), pass.c_str());

        // Waiting for connection
        const int timeoutMs = 5000;
        const int stepMs = 200;
        int waited = 0;
        while (WiFi.status() != WL_CONNECTED && waited < timeoutMs) 
        {
            delay(stepMs);
            waited += stepMs;
        }

        if (WiFi.status() == WL_CONNECTED) 
        {
            std::cout<< "Connected to WiFi: "<< wifi_ssid.c_str() << std::endl;
            std::cout<< "pp: "<< pass.c_str() << std::endl;
            retMsg = "Connected to WiFi successfully!";
            saveWiFiCredentials(wifi_ssid.c_str(), pass.c_str());
        }
        else
        {
            std::cout<< "Failed to connect to WiFi: "<< wifi_ssid.c_str() << std::endl;
            std::cout<< "pp: "<< pass.c_str() << std::endl;
            retMsg = "Failed to connect to WiFi!";
            // Clean up after failure to avoid residual impact on subsequent scans/connections
            WiFi.disconnect(true);
            WiFi.scanDelete();
#ifdef ESP32
            // Optional reset of WiFi driver (in extreme freezing scenarios)
            esp_wifi_stop();
            delay(50);
            esp_wifi_start();
#endif
        }
        lv_indev_wait_release(lv_indev_get_act());
        _ui_screen_change(&ui_WiFi, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_WiFi_screen_init);

        obj_status = lv_label_create(ui_WiFi);
        //lv_set_color(aa, lv_color_hex(0xFF0000));
        lv_label_set_text(obj_status, retMsg.c_str());
        lv_obj_set_style_text_color(obj_status, lv_color_hex(0xFF0000), 0);
        lv_obj_align(obj_status, LV_ALIGN_CENTER, 0, 0);
        lv_timer_create(label_hide_timer_cb, 3000, obj_status);
        return;
    }

    btn_state_t * state = &btn_states[btn_id];
    // Check if the same button is clicked continuously within a certain period of time
    if (last_btn_id == btn_id && (current_time - state->last_click_time) < 1500) 
    {
        // Continuously click the same button: delete the previous character and enter a new character
        delete_last_char();
        state->click_count = (state->click_count + 1) % state->chars_len;
    } 
    else
    {
        // Click a different button or timeout: reset state, enter new characters
        state->click_count = 0;
    }
    
    // Get the character that should be entered
    char current_char = state->chars[state->click_count];
    add_char_to_textarea(current_char);
    
    // Update Status
    state->last_click_time = current_time;
    last_btn_id = btn_id;
}

// Creating a 3×4 keyboard layout
void create_3x4_keyboard(lv_obj_t * parent) 
{
    // Creating a text input area
    textarea = lv_textarea_create(parent);

    // Before entering each time, you must first clear the last input character
    String text = lv_textarea_get_text(textarea);
    if (!text.isEmpty())
    {
        lv_textarea_set_text(textarea, "");
    }
    
    lv_obj_set_size(textarea, 300, 60);
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_one_line(textarea, true);
    
    static lv_coord_t grid_col_dsc[] = {50, 50, 50, 50,50, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {40, 40, 40, LV_GRID_TEMPLATE_LAST};
    
    lv_obj_t * grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 300, 165);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, grid_col_dsc, grid_row_dsc);

    // Transparent background, remove borders/outlines and padding
    lv_obj_set_style_bg_opa(grid, LV_OPA_0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_outline_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);

    // Disable scrolling and hide the scroll bar
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    // Horizontally center: align to the bottom of the textarea with an x ​​offset of 0 (change to your desired vertical spacing)
    lv_obj_align_to(grid, textarea, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Create 12 buttons (3×4)
    for (int row = 0; row < 3; row++) 
    {
        for (int col = 0; col < 5; col++) 
        {
            int btn_id = row * 5 + col;

            // Skip the original 13 and 14 (we only use the combined 12)
            if (btn_id == 13 || btn_id == 14) continue;

            lv_obj_t * btn = lv_btn_create(grid);
            // If it is a merge button (original index 12), make it span 3 columns
            if (btn_id == 12) 
            {
                // Span columns: Occupy 3 columns starting from col
                lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, col, 3,
                                     LV_GRID_ALIGN_CENTER, row, 1);
                // Give the merge button a wider size
                lv_obj_set_size(btn, 150, 40);
            } 
            else 
            {
                lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, col, 1,
                                     LV_GRID_ALIGN_CENTER, row, 1);
                lv_obj_set_size(btn, 50, 40);
            }
            lv_obj_set_user_data(btn, (void*)(intptr_t)btn_id);
            
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_color(btn, lv_color_hex(0x000000), 0);
            lv_obj_set_style_border_width(btn, 1, 0);

            if (btn_id == 10) 
            { 
                // delete button
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF6666), 0);
                lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
            } 
            else if (btn_id == 11)
            { 
                // Punctuation buttons
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x6666FF), 0);
                lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
            }
            
            // Set button label
            lv_obj_t * label = lv_label_create(btn);
            lv_label_set_text(label, btn_display_text[btn_id]);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(label);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
            
            // event
            lv_obj_add_event_cb(btn, keyboard_btn_event_cb, LV_EVENT_CLICKED, NULL);
        }
    }
    init_button_states();
}