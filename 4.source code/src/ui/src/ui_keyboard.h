// #define LV_USE_GRID 1
// #define LV_GRID_TEMPLATE_LAST 1
// #define LV_LAYOUT_GRID 1

#pragma once
#include <lvgl.h>
#include <string>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h> // tzapu/WiFiManager
#include <Preferences.h> // add near other includes

String wifi_ssid = "";
lv_obj_t *aa = NULL;
static Preferences prefs;
static const char *PREF_NAMESPACE = "meowkit";
static const char *KEY_SSID = "ssid";
static const char *KEY_PASS = "pass";

// 定义3×4键盘字符映射（12个按钮）
static const char * btn_chars[15] = {
    "1abc",   // 按钮0: 1, a, b, c
    "2def",   // 按钮1: 2, d, e, f
    "3ghi",   // 按钮2: 3, g, h, i
    "4jkl",   // 按钮3: 4, j, k, l
    "5mno",   // 按钮4: 5, m, n, o
    "6pqr",  // 按钮5: 6, p, q, r, s
    "7stu",   // 按钮6: 7, t, u, v
    "8vwx",  // 按钮7: 8, w, x, y, z
    "9yz",  // 按钮8: 9, ., ,, !, ?
    "0",      // 按钮9: 0
    " DEL ",  // 按钮10: 删除功能
    " ,.!? ",  // 按钮11: 常用标点符号
    " EN ",     // 按钮12: 确认输入结束
    " EN ",     // 按钮13: 确认输入结束
    " EN "     // 按钮14: 确认输入结束
};

// 按钮显示文本
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

// 按钮状态结构
typedef struct 
{
    uint8_t click_count;    // 当前点击次数
    const char *chars;      // 该按钮对应的所有字符
    uint8_t chars_len;      // 字符数量
    uint32_t last_click_time; // 最后点击时间
} btn_state_t;

static btn_state_t btn_states[15];
static lv_obj_t * textarea; // 文本输入区域
static int8_t last_btn_id = -1; // 记录上一次点击的按钮ID

static void saveWiFiCredentials(const char* ssid, const char* pass) {
    prefs.begin(PREF_NAMESPACE, false);
    prefs.putString(KEY_SSID, ssid ? ssid : "");
    prefs.putString(KEY_PASS, pass ? pass : "");
    prefs.end();
}

// 初始化按钮状态
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

// 删除最后一个字符
void delete_last_char(void) 
{
    lv_textarea_del_char(textarea);
}

// 添加字符到文本区域
void add_char_to_textarea(char c) 
{
    char str[2] = {c, '\0'};
    lv_textarea_add_text(textarea, str);
}

// 定时器回调：3秒后删除提示标签
static void aa_hide_timer_cb(lv_timer_t * timer)
{
    lv_obj_t * lbl = (lv_obj_t *)timer->user_data;
    if (lbl) lv_obj_del(lbl);
    aa = NULL;
    lv_timer_del(timer);
}

// 按钮点击事件处理
static void keyboard_btn_event_cb(lv_event_t * e) 
{
    lv_obj_t * btn = lv_event_get_target(e);
    uint32_t btn_id = (uint32_t)lv_obj_get_user_data(btn);
    uint32_t current_time = lv_tick_get();
    
    if (btn_id >= 13) return;
    
    // 特殊功能按钮处理
    if (btn_id == 10) 
    { 
        // 删除按钮
        delete_last_char();
        return;
    }
    // 特殊功能按钮处理,结束输入
    if (btn_id == 12) 
    { 
      
        String pass = lv_textarea_get_text(textarea); // 假设密码为空
        String retMsg  = ""; 
        WiFi.disconnect(true);       // 断开并尝试清理当前连接
        WiFi.scanDelete();           // 释放上次扫描结果
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.persistent(false);      // 禁止将新凭据写入 flash（按需）
        WiFi.mode(WIFI_STA);
#ifdef ESP32
        // 可选：禁用省电以防握手被中断（需要包含 <esp_wifi.h>）
        esp_wifi_set_ps(WIFI_PS_NONE);
#endif
        WiFi.begin(wifi_ssid.c_str(), pass.c_str());

        // 等待连接（最多 5 秒）
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
                // 失败后再清理，避免残留影响后续扫描/连接
            WiFi.disconnect(true);
            WiFi.scanDelete();
#ifdef ESP32
            // 可选重置 WiFi 驱动（在极端卡死场景）
            esp_wifi_stop();
            delay(50);
            esp_wifi_start();
#endif
        }
        lv_indev_wait_release(lv_indev_get_act());
        _ui_screen_change(&ui_WiFi, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_WiFi_screen_init);

        aa = lv_label_create(ui_WiFi);
        //lv_set_color(aa, lv_color_hex(0xFF0000));
        lv_label_set_text(aa, retMsg.c_str());
        // 红色文字
        lv_obj_set_style_text_color(aa, lv_color_hex(0xFF0000), 0);
        // 居中显示（按需调整位置）
        lv_obj_align(aa, LV_ALIGN_CENTER, 0, 0);
        // 3秒后自动消失
        lv_timer_create(aa_hide_timer_cb, 3000, aa);
        return;
    }

    btn_state_t * state = &btn_states[btn_id];
    // 检查是否是同一个按钮且在一定时间内连续点击
    if (last_btn_id == btn_id && (current_time - state->last_click_time) < 1500) 
    {
        // 连续点击同一个按钮：删除前一个字符，输入新字符
        delete_last_char();
        state->click_count = (state->click_count + 1) % state->chars_len;
    } 
    else
    {
        // 点击不同按钮或超时：重置状态，输入新字符
        state->click_count = 0;
    }
    
    // 获取当前应该输入的字符
    char current_char = state->chars[state->click_count];
    add_char_to_textarea(current_char);
    
    // 更新状态
    state->last_click_time = current_time;
    last_btn_id = btn_id;
}

// 创建3×4键盘布局
void create_3x4_keyboard(lv_obj_t * parent) 
{
    // 创建文本输入区域
    textarea = lv_textarea_create(parent);

     // 每一次进入之前都要先清除上一次输入的字符
    String text = lv_textarea_get_text(textarea);
    if (!text.isEmpty())
    {
        lv_textarea_set_text(textarea, "");
    }
    
    lv_obj_set_size(textarea, 300, 60);
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 10);
    lv_textarea_set_one_line(textarea, true);
    
    // 创建按钮容器 - 3行4列
    static lv_coord_t grid_col_dsc[] = {50, 50, 50, 50,50, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {40, 40, 40, LV_GRID_TEMPLATE_LAST};
    
    // lv_obj_t * grid = lv_obj_create(parent);
    // lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    // lv_obj_set_grid_dsc_array(grid, grid_col_dsc, grid_row_dsc);
    // lv_obj_set_size(grid, 300, 165);
    // lv_obj_align_to(grid, textarea, LV_ALIGN_OUT_BOTTOM_MID, 10,10);
    // lv_obj_set_style_bg_opa(grid, LV_OPA_0, 0); // 透明背景
    
    lv_obj_t * grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 300, 165);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(grid, grid_col_dsc, grid_row_dsc);

    // 透明背景、去掉边框/轮廓和内边距
    lv_obj_set_style_bg_opa(grid, LV_OPA_0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_outline_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);

    // 取消可滚动并隐藏滚动条（避免出现滑动条）
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

    // 水平居中：与 textarea 底部对齐且 x 偏移为 0（改成你需要的垂直间距）
    lv_obj_align_to(grid, textarea, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // 创建12个按钮（3×4）
    for (int row = 0; row < 3; row++) 
    {
        for (int col = 0; col < 5; col++) 
        {
            int btn_id = row * 5 + col;

            // 跳过原本的 13、14（我们只用一个合并后的 12）
            if (btn_id == 13 || btn_id == 14) continue;

            lv_obj_t * btn = lv_btn_create(grid);
            // 如果是合并按钮（原来的索引12），让它跨 3 列
            if (btn_id == 12) 
            {
                // 跨列：从 col 开始占 3 列
                lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, col, 3,
                                     LV_GRID_ALIGN_CENTER, row, 1);
                // 给合并按钮一个更宽的尺寸（可根据实际视觉调整）
                lv_obj_set_size(btn, 150, 40);
            } 
            else 
            {
                lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, col, 1,
                                     LV_GRID_ALIGN_CENTER, row, 1);
                lv_obj_set_size(btn, 50, 40);
            }
            lv_obj_set_user_data(btn, (void*)(intptr_t)btn_id);
            
            // 设置按钮样式
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_color(btn, lv_color_hex(0x000000), 0);
            lv_obj_set_style_border_width(btn, 1, 0);
            
            // 特殊功能按钮样式
            if (btn_id == 10) 
            { 
                // 删除按钮
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF6666), 0);
                lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
            } 
            else if (btn_id == 11)
            { 
                // 标点符号按钮
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x6666FF), 0);
                lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
            }
            
            // 设置按钮标签
            lv_obj_t * label = lv_label_create(btn);
            lv_label_set_text(label, btn_display_text[btn_id]);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(label);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
            
            // 添加事件处理器
            lv_obj_add_event_cb(btn, keyboard_btn_event_cb, LV_EVENT_CLICKED, NULL);
        }
    }
    
    init_button_states();
}