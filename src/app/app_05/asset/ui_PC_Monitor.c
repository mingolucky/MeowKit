// Original 320 x 240 artwork, read directly from memory-mapped Flash.
#include "ui.h"
#include <string.h>
lv_obj_t *ui_PC_Monitor;
lv_obj_t *ui_cpu_name, *ui_gpu_name, *ui_gpu_brand;
lv_obj_t *ui_cpu_temp, *ui_cpu_percent, *ui_gpu_temp, *ui_gpu_percent;
lv_obj_t *ui_cpu_mhz, *ui_gpu_mhz, *ui_gpu_ram, *ui_ram_total, *ui_ram_percent;
lv_obj_t *ui_gpu_fan_load, *ui_gpu_fan_rpm;
lv_obj_t *ui_cpu_temp_bar, *ui_cpu_load_bar, *ui_gpu_temp_bar, *ui_gpu_load_bar, *ui_ram_bar;
static lv_obj_t *status_label;
static lv_obj_t *label(int x, int y, int w, const char *text,
                       const lv_font_t *font, uint32_t color) {
    lv_obj_t *obj = lv_label_create(ui_PC_Monitor);
    lv_obj_set_pos(obj, x, y); lv_obj_set_width(obj, w);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_label_set_text(obj, text); return obj;
}
static lv_obj_t *number(int x, int y, int w) {
    lv_obj_t *obj = label(x, y, w, "--", &ui_font_pc_temp, 0xBDE800);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0); return obj;
}
static void image(const lv_img_dsc_t *resource, int x, int y) {
    lv_obj_t *obj = lv_img_create(ui_PC_Monitor);
    lv_img_set_src(obj, resource); lv_obj_set_pos(obj, x, y);
}
// Reveal fixed-color segments proportionally, without animations or decoding.
static lv_obj_t *meter(int x, int y, int w, int h, bool vertical) {
    lv_obj_t *obj = lv_obj_create(ui_PC_Monitor);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, h);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    if (vertical) lv_obj_add_flag(obj, LV_OBJ_FLAG_USER_1);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x303236), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    // Cool low-range colors contrast with the lime artwork; warm high ranges.
    static const uint32_t colors[] = {0x22D3EE,0x4D9FFF,0x9B8AFB,0xFFBE55,0xFF706B};
    for (int i = 0; i < 5; ++i) {
        lv_obj_t *fill = lv_obj_create(obj);
        lv_obj_remove_style_all(fill); lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(fill, lv_color_hex(colors[i]), 0);
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
        lv_obj_add_flag(fill, LV_OBJ_FLAG_HIDDEN);
    }
    return obj;
}
void ui_pc_monitor_meter(lv_obj_t *obj, int value) {
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    int w = lv_obj_get_style_width(obj, 0), h = lv_obj_get_style_height(obj, 0);
    int count = lv_obj_get_child_cnt(obj);
    bool vertical = lv_obj_has_flag(obj, LV_OBJ_FLAG_USER_1);
    int length = vertical ? h : w;
    for (int i = 0; i < count; ++i) {
        lv_obj_t *fill = lv_obj_get_child(obj, i);
        int start = length * i / count;
        int size = length * value / 100 - start;
        int max = length * (i + 1) / count - start;
        if (size > max) size = max;
        if (size <= 0) { lv_obj_add_flag(fill, LV_OBJ_FLAG_HIDDEN); continue; }
        lv_obj_clear_flag(fill, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(fill, vertical ? 0 : start, vertical ? h - start - size : 0);
        lv_obj_set_size(fill, vertical ? w : size, vertical ? size : h);
    }
}
void ui_pc_monitor_init(void) {
    ui_PC_Monitor = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_PC_Monitor, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_PC_Monitor, lv_color_black(), 0);
    lv_obj_set_style_pad_all(ui_PC_Monitor, 0, 0);
    image(&pc_monitor_background, 0, 0);
    image(&pc_monitor_overlay, 10, 2);
    ui_cpu_name = label(66, 16, 120, "CPU", &lv_font_montserrat_10, 0xFFFFFF);
    lv_label_set_long_mode(ui_cpu_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_speed(ui_cpu_name, 20, 0);
    ui_cpu_mhz = number(65, 31, 97);
    label(250, 16, 51, "RAM", &lv_font_montserrat_10, 0x93998D);
    ui_ram_total = number(246, 31, 40);
    // Original artwork supplies MHz, GB, Celsius and percent units.
    ui_cpu_temp_bar = meter(19, 72, 100, 20, false);
    ui_cpu_load_bar = meter(19, 102, 100, 20, false);
    ui_cpu_temp = number(124, 67, 49);
    ui_cpu_percent = number(124, 97, 49);
    ui_ram_bar = meter(200, 71, 40, 50, true);
    label(250, 72, 55, "USED", &lv_font_montserrat_10, 0x93998D);
    label(250, 83, 55, "MEMORY", &lv_font_montserrat_10, 0x93998D);
    ui_ram_percent = number(245, 97, 43);
    label(66, 140, 60, "GPU", &lv_font_montserrat_10, 0x93998D);
    ui_gpu_brand = label(65, 152, 62, "", &lv_font_montserrat_10, 0xFFFFFF);
    ui_gpu_name = label(65, 164, 62, "GPU", &lv_font_montserrat_10, 0xFFFFFF);
    lv_label_set_long_mode(ui_gpu_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_speed(ui_gpu_name, 20, 0);
    ui_gpu_ram = label(19, 183, 90, "-- GB", &lv_font_montserrat_10, 0xFFFFFF);
    // Right-aligned digits end 8 px earlier, leaving the artwork's MHz clear.
    ui_gpu_mhz = number(18, 197, 84);
    label(140, 140, 63, "FAN LOAD", &lv_font_montserrat_10, 0x93998D);
    label(140, 154, 68, "FAN SPEED", &lv_font_montserrat_10, 0x93998D);
    ui_gpu_fan_load = label(211, 140, 85, "--", &lv_font_montserrat_10, 0xFFFFFF);
    ui_gpu_fan_rpm = label(211, 154, 85, "--", &lv_font_montserrat_10, 0xFFFFFF);
    ui_gpu_temp_bar = meter(140, 171, 100, 20, false);
    ui_gpu_load_bar = meter(140, 201, 100, 20, false);
    ui_gpu_temp = number(245, 166, 43);
    ui_gpu_percent = number(245, 196, 43);
    status_label = label(10, 230, 302, "", &lv_font_montserrat_10, 0xFFFFFF);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_scr_load(ui_PC_Monitor);
}
void ui_pc_monitor_status(const char *text, bool live) {
    if (!status_label) return;
    if (live) { lv_obj_add_flag(status_label, LV_OBJ_FLAG_HIDDEN); return; }
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
    if (strcmp(lv_label_get_text(status_label), text)) lv_label_set_text(status_label, text);
}
void ui_pc_monitor_destroy(void) {
    if (!ui_PC_Monitor || lv_scr_act() == ui_PC_Monitor) return;
    lv_obj_del(ui_PC_Monitor); ui_PC_Monitor = NULL; status_label = NULL;
    ui_cpu_name = ui_gpu_name = ui_gpu_brand = ui_cpu_temp = ui_cpu_percent = ui_gpu_temp = ui_gpu_percent = NULL;
    ui_cpu_mhz = ui_gpu_mhz = ui_gpu_ram = ui_ram_total = ui_ram_percent = NULL;
    ui_gpu_fan_load = ui_gpu_fan_rpm = NULL;
    ui_cpu_temp_bar = ui_cpu_load_bar = ui_gpu_temp_bar = ui_gpu_load_bar = ui_ram_bar = NULL;
}
