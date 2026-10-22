/**
 * @file app_badusb.cpp
 * @author Mingo
 * @brief AppBadUSB -- Bad USB v2.0 (Momentum-Firmware DuckyScript port)
 *        Full DuckyScript parser + USB HID keyboard/mouse/consumer-control
 *        via ESP32-S3 TinyUSB.  TUI style (same as App11).
 * @version 2.0
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 *
 * Ported from: https://github.com/Next-Flip/Momentum-Firmware
 *   helpers/ducky_script.c, ducky_script_commands.c, ducky_script_keycodes.c
 */
#include "badusb.h"
#include "badusb_parse_utils.h"
#include "../app_common/hp_ui.h"
#include <SD_MMC.h>
#include <FS.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>

static inline bool _hidConnected() { return usb_hid_connected() != 0; }

/* BLE HID - isolated wrapper avoids header collisions with USBHIDKeyboard. */
#include "badusb_ble.h"
#include "../../system/bluetooth_manager.h"

/* ================================================================ */
/*  LVGL number fonts  (read-only flash data, safe from any core)
 * ================================================================ */
extern "C" {
#include <lvgl.h>
extern const lv_font_t ui_font_number_60;  /* 42 px line height, digits + ./:  */
extern const lv_font_t ui_font_number_24;  /* 23 px line height, full ASCII     */
}

/* ── LVGL 4-bpp glyph renderer via LovyanGFX ──────────────────────
 * Measures advance width of a string in the given LVGL font (pixels). */
static int32_t lvglStrW(const lv_font_t* font, const char* s)
{
    int32_t w = 0;
    while (*s) {
        lv_font_glyph_dsc_t g;
        if (lv_font_get_glyph_dsc(font, &g, (uint32_t)(unsigned char)*s, 0))
            w += (int32_t)g.adv_w;
        ++s;
    }
    return w;
}

/* Draw string using LVGL font through LovyanGFX pixel-level API.
 * x, y = top-left of text line (LVGL convention).
 * Threshold: draw pixel when 4-bpp alpha nibble >= 8 (≥50% coverage). */
static void lvglDrawStr(LovyanGFX& lcd, int x, int y,
                        const lv_font_t* font, const char* s,
                        uint16_t color)
{
    const int lh = (int)font->line_height;
    const int bl = (int)font->base_line;
    int px = x;
    while (*s) {
        uint32_t cp = (uint32_t)(unsigned char)*s++;
        lv_font_glyph_dsc_t g;
        if (!lv_font_get_glyph_dsc(font, &g, cp, 0)) continue;

        const uint8_t* bmp = lv_font_get_glyph_bitmap(font, cp);
        if (bmp && g.box_w > 0 && g.box_h > 0) {
            /* Glyph top-left in screen coords:
             *   gy = y + (line_height - base_line) - ofs_y - box_h  */
            int gx = px + (int)g.ofs_x;
            int gy = y  + (lh - bl) - (int)g.ofs_y - (int)g.box_h;
            for (int row = 0; row < (int)g.box_h; ++row) {
                for (int col = 0; col < (int)g.box_w; ++col) {
                    int idx = row * (int)g.box_w + col;
                    uint8_t nibble = (idx & 1)
                        ? (bmp[idx >> 1] & 0x0F)
                        : ((bmp[idx >> 1] >> 4) & 0x0F);
                    if (nibble >= 8)
                        lcd.drawPixel(gx + col, gy + row, color);
                }
            }
        }
        px += (int)g.adv_w;
    }
}

/* ================================================================ */
/*  Embedded PNG status icons  (140×50 px, <512 bytes each)
 * ================================================================ */
static const uint8_t kIconConnected[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x00, 0x32, 0x04, 0x03, 0x00, 0x00, 0x00, 0x7C, 0x2E, 0x2E,
    0x08, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8F, 0x0B, 0xFC, 0x61,
    0x05, 0x00, 0x00, 0x00, 0x01, 0x73, 0x52, 0x47, 0x42, 0x00, 0xAE, 0xCE, 0x1C, 0xE9, 0x00, 0x00,
    0x00, 0x24, 0x50, 0x4C, 0x54, 0x45, 0x4C, 0x69, 0x71, 0xBB, 0xE7, 0x00, 0xBB, 0xE7, 0x00, 0xBB,
    0xE7, 0x00, 0xBB, 0xE7, 0x00, 0xBB, 0xE7, 0x00, 0xBB, 0xE7, 0x00, 0xBB, 0xE7, 0x00, 0xBB, 0xE7,
    0x00, 0xBB, 0xE7, 0x00, 0xBB, 0xE7, 0x00, 0xBB, 0xE7, 0x00, 0x9F, 0x1A, 0xEE, 0xF1, 0x00, 0x00,
    0x00, 0x0B, 0x74, 0x52, 0x4E, 0x53, 0x00, 0x7F, 0x10, 0xA0, 0xD0, 0x3B, 0xC0, 0xED, 0x20, 0x59,
    0xB0, 0x00, 0xFC, 0x4B, 0x73, 0x00, 0x00, 0x01, 0x49, 0x49, 0x44, 0x41, 0x54, 0x48, 0xC7, 0xED,
    0x96, 0xBD, 0x4E, 0xC3, 0x30, 0x14, 0x46, 0xAD, 0x54, 0x2C, 0x78, 0xA1, 0x13, 0xAA, 0x58, 0x32,
    0x54, 0xAA, 0xE8, 0x44, 0x57, 0xBA, 0xB1, 0x76, 0xE9, 0x86, 0xC4, 0xC4, 0xD4, 0xBD, 0x5B, 0x2B,
    0x16, 0x3F, 0x02, 0xA8, 0x03, 0x8C, 0x88, 0x57, 0x30, 0xB4, 0xAA, 0xBE, 0x97, 0x23, 0x49, 0x9D,
    0x2A, 0x71, 0xD2, 0xD4, 0xBE, 0xF7, 0x0E, 0x0C, 0x7C, 0x9B, 0xA5, 0xF8, 0xE8, 0xE4, 0xFA, 0xE7,
    0x5A, 0xA9, 0xB8, 0x24, 0x9B, 0xC5, 0xC4, 0x28, 0x6E, 0x92, 0x29, 0x80, 0xED, 0x9C, 0x8B, 0x79,
    0x44, 0x9E, 0x3D, 0x93, 0x72, 0xF1, 0x52, 0x60, 0xF0, 0xCA, 0xC3, 0x8C, 0x0E, 0x14, 0xFC, 0xF0,
    0x30, 0x37, 0x0E, 0xB3, 0xE5, 0x61, 0xDC, 0x3F, 0x01, 0xB4, 0x22, 0x6F, 0x8E, 0xF3, 0x5D, 0xAE,
    0x28, 0x94, 0x77, 0xF8, 0x79, 0x26, 0x50, 0xC6, 0x90, 0xC0, 0x24, 0x33, 0x11, 0xCC, 0xB0, 0x49,
    0xC1, 0xA7, 0x88, 0x0C, 0xA1, 0xC4, 0xB9, 0xCC, 0xEE, 0xC3, 0x5B, 0x70, 0x43, 0x92, 0x79, 0x60,
    0x6F, 0xBF, 0xCB, 0x6C, 0xD6, 0x77, 0xEA, 0x95, 0x69, 0x15, 0x8D, 0x99, 0x56, 0x64, 0xE8, 0x47,
    0xB3, 0x57, 0x95, 0x29, 0x2F, 0x8A, 0x75, 0x69, 0x3A, 0x27, 0xC9, 0x64, 0x95, 0xBA, 0xCE, 0xC6,
    0xB6, 0x2C, 0xB0, 0xB6, 0x86, 0x24, 0x93, 0xE5, 0xED, 0x7E, 0x70, 0x1C, 0x6B, 0x04, 0x72, 0x3C,
    0x19, 0x2F, 0x1A, 0x61, 0x9C, 0xA6, 0x8C, 0x8F, 0x09, 0xE2, 0x74, 0xCB, 0x14, 0x18, 0xD8, 0x34,
    0xE4, 0x68, 0x77, 0xC8, 0x1C, 0x30, 0x58, 0xE6, 0x5F, 0x80, 0x9F, 0x9C, 0x03, 0x19, 0x0E, 0x44,
    0x38, 0x7F, 0x09, 0x23, 0xF3, 0x53, 0xCB, 0x34, 0xF0, 0xDA, 0xB2, 0x67, 0x17, 0x3C, 0xEC, 0xDE,
    0xBA, 0x3B, 0x8D, 0x09, 0xA3, 0x74, 0xE9, 0xE8, 0xA0, 0x4D, 0xDC, 0xAA, 0xD3, 0x77, 0x31, 0x11,
    0x47, 0xB3, 0x45, 0xA7, 0xDA, 0xC5, 0x75, 0x0C, 0xA5, 0xAE, 0x53, 0xC3, 0x58, 0x13, 0xDB, 0x63,
    0x6C, 0x0B, 0xA6, 0x67, 0xE2, 0x9B, 0xCC, 0x57, 0x13, 0x43, 0x6A, 0x79, 0x93, 0xA2, 0xB4, 0x1C,
    0x4C, 0x4B, 0x03, 0x26, 0x3D, 0x94, 0x86, 0x32, 0x98, 0xA6, 0x0E, 0xED, 0xD9, 0x36, 0x96, 0xC1,
    0xA8, 0x27, 0x19, 0x8C, 0xBA, 0xED, 0xB3, 0x57, 0xAA, 0x92, 0x7F, 0xCC, 0xE9, 0x2C, 0x5C, 0xEA,
    0x98, 0x5F, 0xC5, 0xCB, 0xA0, 0xDA, 0xF7, 0x5D, 0x92, 0xA2, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
    0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};
static const size_t kIconConnectedLen = sizeof(kIconConnected);

static const uint8_t kIconUnconnected[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x00, 0x32, 0x04, 0x03, 0x00, 0x00, 0x00, 0x7C, 0x2E, 0x2E,
    0x08, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8F, 0x0B, 0xFC, 0x61,
    0x05, 0x00, 0x00, 0x00, 0x01, 0x73, 0x52, 0x47, 0x42, 0x00, 0xAE, 0xCE, 0x1C, 0xE9, 0x00, 0x00,
    0x00, 0x1E, 0x50, 0x4C, 0x54, 0x45, 0x4C, 0x69, 0x71, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7,
    0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7,
    0xB7, 0xB7, 0xB7, 0xB7, 0x0F, 0x88, 0xEB, 0xBD, 0x00, 0x00, 0x00, 0x09, 0x74, 0x52, 0x4E, 0x53,
    0x00, 0x7F, 0xC9, 0x10, 0xA4, 0x3B, 0xED, 0x20, 0x59, 0xD8, 0xC3, 0x08, 0xA9, 0x00, 0x00, 0x01,
    0x42, 0x49, 0x44, 0x41, 0x54, 0x48, 0xC7, 0xED, 0xD6, 0xB1, 0x6E, 0xC2, 0x30, 0x14, 0x05, 0x50,
    0x4B, 0xA1, 0x83, 0xB7, 0xB2, 0x54, 0xED, 0x56, 0xD1, 0xA9, 0x5B, 0x84, 0x10, 0xEA, 0x08, 0x6A,
    0x17, 0x36, 0x86, 0xAA, 0x3B, 0x53, 0xDB, 0xCD, 0x9F, 0x00, 0x12, 0x2A, 0x3B, 0x72, 0x11, 0xF7,
    0x6F, 0x1B, 0x07, 0x07, 0x25, 0x4E, 0x48, 0xED, 0xF7, 0xDE, 0xD0, 0xA1, 0x77, 0x8B, 0x20, 0x47,
    0x37, 0x76, 0x6C, 0x47, 0xA9, 0xB4, 0x64, 0xDB, 0xD1, 0xDC, 0x28, 0x6E, 0xB2, 0x05, 0x80, 0xFD,
    0x92, 0xCB, 0xBC, 0xC1, 0xE5, 0xC8, 0x54, 0xAE, 0x3E, 0x4B, 0x06, 0x6B, 0x1E, 0x33, 0x3D, 0x29,
    0xF8, 0xE6, 0x31, 0x77, 0x9E, 0xD9, 0xF3, 0x18, 0xFF, 0x4C, 0x00, 0x6D, 0x90, 0xB7, 0xE7, 0xFB,
    0x7D, 0xAE, 0x29, 0xCA, 0x17, 0xC2, 0xDC, 0x13, 0x94, 0x09, 0x24, 0x98, 0xEC, 0x51, 0x84, 0x19,
    0xB7, 0x15, 0xCC, 0x44, 0xCA, 0x10, 0x86, 0xD8, 0x95, 0x39, 0xEC, 0x82, 0x09, 0x37, 0xA4, 0x32,
    0x0F, 0xEC, 0xD7, 0x6F, 0xE0, 0xEE, 0xCA, 0x83, 0x61, 0x7A, 0x4F, 0x66, 0x16, 0xB5, 0x32, 0xF4,
    0xA5, 0xD9, 0x28, 0x53, 0x6D, 0x14, 0x1F, 0xD5, 0x8F, 0x4B, 0x52, 0x99, 0x62, 0xA4, 0x6E, 0x8A,
    0x6B, 0x5B, 0x0D, 0xB0, 0xB6, 0x86, 0x54, 0xA6, 0xC8, 0xE6, 0xE5, 0xF6, 0x7C, 0xAD, 0x11, 0xE9,
    0x04, 0x65, 0x82, 0x68, 0xC4, 0x39, 0xED, 0x32, 0x21, 0x13, 0xE5, 0xF4, 0x97, 0x29, 0x19, 0xD8,
    0x3C, 0x66, 0x69, 0xF7, 0x94, 0x39, 0x31, 0x58, 0xB9, 0x7F, 0x80, 0x1F, 0xE7, 0x40, 0xC6, 0x81,
    0x88, 0xF3, 0x97, 0x18, 0x99, 0x87, 0x5A, 0xE5, 0x91, 0xDB, 0x96, 0xFD, 0x75, 0xC2, 0xE3, 0xF6,
    0xAD, 0xD9, 0x65, 0x26, 0x4E, 0xE9, 0xAB, 0xA3, 0xA3, 0x5E, 0xE2, 0xCE, 0x3A, 0x43, 0x1F, 0x93,
    0xB0, 0x34, 0x3B, 0xEA, 0xD4, 0x4F, 0x71, 0x9D, 0xA2, 0x34, 0xEB, 0x34, 0x18, 0x6B, 0x52, 0xCF,
    0x18, 0xDB, 0xC1, 0x0C, 0x4C, 0xFA, 0x21, 0xF3, 0xDC, 0x66, 0x48, 0x47, 0xDE, 0xBC, 0x1C, 0x5A,
    0x0E, 0xD3, 0x71, 0x00, 0x93, 0x3E, 0x94, 0xC6, 0x32, 0x4C, 0xBB, 0x0E, 0xED, 0xB3, 0x6D, 0x22,
    0xC3, 0xA8, 0x57, 0x19, 0x46, 0x3D, 0x0D, 0xD9, 0x33, 0x55, 0xCB, 0x3F, 0x73, 0x39, 0x23, 0x9F,
    0x26, 0xF3, 0x03, 0xAF, 0xB0, 0x28, 0xA1, 0xDB, 0x2C, 0xD3, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x49,
    0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};
static const size_t kIconUnconnectedLen = sizeof(kIconUnconnected);

static const char* SCRIPT_DIR = "/badusb/scripts";
static const char* LAYOUT_DIR = "/badusb/layouts";

/* --- Layout (bound to hp_ui shared chrome) --- */
static constexpr int SCR_W        = hp::W;
static constexpr int SCR_H        = hp::H;
static constexpr int HDR_H        = hp::CON_Y0;          // 18
static constexpr int FTR_H        = hp::H - hp::FTR_SEP; // 18
static constexpr int ITEM_H       = hp::ITEM_H;          // 18
static constexpr int MENU_Y0      = hp::CON_Y0 + 2;      // 20
static constexpr int MENU_VISIBLE = hp::LIST_VIS;        // 11

/* ================================================================ */
/*  Raw HID keycodes for keypad digits (used by ALTCHAR).
 * ================================================================ */
#define HID_KEYPAD_0 0x62
#define HID_KEYPAD_1 0x59
#define HID_KEYPAD_2 0x5A
#define HID_KEYPAD_3 0x5B
#define HID_KEYPAD_4 0x5C
#define HID_KEYPAD_5 0x5D
#define HID_KEYPAD_6 0x5E
#define HID_KEYPAD_7 0x5F
#define HID_KEYPAD_8 0x60
#define HID_KEYPAD_9 0x61
static const uint8_t s_numpadKeys[10] = {
    HID_KEYPAD_0, HID_KEYPAD_1, HID_KEYPAD_2, HID_KEYPAD_3, HID_KEYPAD_4,
    HID_KEYPAD_5, HID_KEYPAD_6, HID_KEYPAD_7, HID_KEYPAD_8, HID_KEYPAD_9,
};

/* ================================================================ */
/*  Consumer-control usage codes (supplement Arduino header)
 * ================================================================ */
#ifndef CC_POWER
#define CC_POWER         0x0030
#endif
#ifndef CC_RESET
#define CC_RESET         0x0031
#endif
#ifndef CC_SLEEP
#define CC_SLEEP         0x0032
#endif
#define CC_SNAPSHOT      0x0065
#define CC_PLAY          0x00B0
#define CC_PAUSE         0x00B1
#define CC_NEXT_TRACK    0x00B5
#define CC_PREV_TRACK    0x00B6
#define CC_STOP          0x00B7
#define CC_EJECT         0x00B8
#define CC_PLAY_PAUSE    0x00CD
#define CC_MUTE          0x00E2
#define CC_VOL_UP        0x00E9
#define CC_VOL_DOWN      0x00EA
#define CC_BRIGHT_UP     0x006F
#define CC_BRIGHT_DOWN   0x0070
#define CC_EXIT          0x0094
#define CC_HOME          0x0223
#define CC_BACK          0x0224
#define CC_FORWARD       0x0225
#define CC_REFRESH       0x0227
#define CC_LOGOFF        0x019C
#define CC_FN_GLOBE      0x029D   // macOS Globe / Fn key

/* ================================================================ */
/*  Stub KEY_* constants when TinyUSB is disabled
 * ================================================================ */
#if ARDUINO_USB_MODE != 0
#define KEY_LEFT_CTRL   0x80
#define KEY_LEFT_SHIFT  0x81
#define KEY_LEFT_ALT    0x82
#define KEY_LEFT_GUI    0x83
#define KEY_RIGHT_CTRL  0x84
#define KEY_RIGHT_SHIFT 0x85
#define KEY_RIGHT_ALT   0x86
#define KEY_RIGHT_GUI   0x87
#define KEY_RETURN      0xB0
#define KEY_ESC         0xB1
#define KEY_BACKSPACE   0xB2
#define KEY_TAB         0xB3
#define KEY_CAPS_LOCK   0xC1
#define KEY_DELETE      0xD4
#define KEY_INSERT      0xD1
#define KEY_HOME        0xD2
#define KEY_END         0xD5
#define KEY_PAGE_UP     0xD3
#define KEY_PAGE_DOWN   0xD6
#define KEY_UP_ARROW    0xDA
#define KEY_DOWN_ARROW  0xD9
#define KEY_LEFT_ARROW  0xD8
#define KEY_RIGHT_ARROW 0xD7
#define KEY_F1  0xC2
#define KEY_F2  0xC3
#define KEY_F3  0xC4
#define KEY_F4  0xC5
#define KEY_F5  0xC6
#define KEY_F6  0xC7
#define KEY_F7  0xC8
#define KEY_F8  0xC9
#define KEY_F9  0xCA
#define KEY_F10 0xCB
#define KEY_F11 0xCC
#define KEY_F12 0xCD
#define KEY_F13 0xF0
#define KEY_F14 0xF1
#define KEY_F15 0xF2
#define KEY_F16 0xF3
#define KEY_F17 0xF4
#define KEY_F18 0xF5
#define KEY_F19 0xF6
#define KEY_F20 0xF7
#define KEY_F21 0xF8
#define KEY_F22 0xF9
#define KEY_F23 0xFA
#define KEY_F24 0xFB
#define KEY_NUM_LOCK    0xDB
#define KEY_SCROLL_LOCK 0xCF
#define KEY_PRTSCR      0xCE
#define KEY_PAUSE       0xD0
#define KEY_MENU        0xED
#define MOUSE_LEFT   0x01
#define MOUSE_RIGHT  0x02
#define MOUSE_MIDDLE 0x04
#endif /* ARDUINO_USB_MODE != 0 */

/* Keys not defined in Arduino USBHIDKeyboard.h for any mode */
#ifndef KEY_NUM_LOCK
#define KEY_NUM_LOCK    0xDB
#endif
#ifndef KEY_SCROLL_LOCK
#define KEY_SCROLL_LOCK 0xCF
#endif
#ifndef KEY_PRTSCR
#define KEY_PRTSCR      0xCE
#endif
#ifndef KEY_PAUSE
#define KEY_PAUSE       0xD0
#endif
#ifndef KEY_MENU
#define KEY_MENU        0xED
#endif

/* ================================================================ */
/*  Lookup tables -- 1:1 from Momentum ducky_script_keycodes.c
 * ================================================================ */
struct KEntry { const char* name; uint8_t code; };
struct MEntry { const char* name; uint16_t code; };

static const KEntry s_mods[] = {
    {"CTRL",        KEY_LEFT_CTRL},
    {"CONTROL",     KEY_LEFT_CTRL},
    {"SHIFT",       KEY_LEFT_SHIFT},
    {"ALT",         KEY_LEFT_ALT},
    {"GUI",         KEY_LEFT_GUI},
    {"WINDOWS",     KEY_LEFT_GUI},
    {"COMMAND",     KEY_LEFT_GUI},
    {"OPTION",      KEY_LEFT_ALT},
    {"RCTRL",       KEY_RIGHT_CTRL},
    {"RSHIFT",      KEY_RIGHT_SHIFT},
    {"RALT",        KEY_RIGHT_ALT},
    {"RGUI",        KEY_RIGHT_GUI},
    {nullptr, 0}
};

static const KEntry s_keys[] = {
    {"DOWNARROW",   KEY_DOWN_ARROW},
    {"DOWN",        KEY_DOWN_ARROW},
    {"LEFTARROW",   KEY_LEFT_ARROW},
    {"LEFT",        KEY_LEFT_ARROW},
    {"RIGHTARROW",  KEY_RIGHT_ARROW},
    {"RIGHT",       KEY_RIGHT_ARROW},
    {"UPARROW",     KEY_UP_ARROW},
    {"UP",          KEY_UP_ARROW},
    {"ENTER",       KEY_RETURN},
    {"RETURN",      KEY_RETURN},
    {"BREAK",       KEY_PAUSE},
    {"PAUSE",       KEY_PAUSE},
    {"CAPSLOCK",    KEY_CAPS_LOCK},
    {"DELETE",      KEY_DELETE},
    {"DEL",         KEY_DELETE},
    {"BACKSPACE",   KEY_BACKSPACE},
    {"END",         KEY_END},
    {"ESC",         KEY_ESC},
    {"ESCAPE",      KEY_ESC},
    {"HOME",        KEY_HOME},
    {"INSERT",      KEY_INSERT},
    {"NUMLOCK",     KEY_NUM_LOCK},
    {"PAGEUP",      KEY_PAGE_UP},
    {"PAGEDOWN",    KEY_PAGE_DOWN},
    {"PRINTSCREEN", KEY_PRTSCR},
    {"SCROLLLOCK",  KEY_SCROLL_LOCK},
    {"SPACE",       ' '},
    {"TAB",         KEY_TAB},
    {"MENU",        KEY_MENU},
    {"APP",         KEY_MENU},
    {"F1",  KEY_F1},  {"F2",  KEY_F2},  {"F3",  KEY_F3},
    {"F4",  KEY_F4},  {"F5",  KEY_F5},  {"F6",  KEY_F6},
    {"F7",  KEY_F7},  {"F8",  KEY_F8},  {"F9",  KEY_F9},
    {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12},
    {"F13", KEY_F13}, {"F14", KEY_F14}, {"F15", KEY_F15},
    {"F16", KEY_F16}, {"F17", KEY_F17}, {"F18", KEY_F18},
    {"F19", KEY_F19}, {"F20", KEY_F20}, {"F21", KEY_F21},
    {"F22", KEY_F22}, {"F23", KEY_F23}, {"F24", KEY_F24},
    {nullptr, 0}
};

static const MEntry s_media[] = {
    {"POWER",       CC_POWER},
    {"REBOOT",      CC_RESET},
    {"SLEEP",       CC_SLEEP},
    {"LOGOFF",      CC_LOGOFF},
    {"EXIT",        CC_EXIT},
    {"HOME",        CC_HOME},
    {"BACK",        CC_BACK},
    {"FORWARD",     CC_FORWARD},
    {"REFRESH",     CC_REFRESH},
    {"SNAPSHOT",    CC_SNAPSHOT},
    {"PLAY",        CC_PLAY},
    {"PAUSE",       CC_PAUSE},
    {"PLAY_PAUSE",  CC_PLAY_PAUSE},
    {"NEXT_TRACK",  CC_NEXT_TRACK},
    {"PREV_TRACK",  CC_PREV_TRACK},
    {"STOP",        CC_STOP},
    {"EJECT",       CC_EJECT},
    {"MUTE",        CC_MUTE},
    {"VOLUME_UP",   CC_VOL_UP},
    {"VOLUME_DOWN", CC_VOL_DOWN},
    {"FN",          CC_FN_GLOBE},
    {"BRIGHT_UP",   CC_BRIGHT_UP},
    {"BRIGHT_DOWN", CC_BRIGHT_DOWN},
    {nullptr, 0}
};

static const KEntry s_mouseKeys[] = {
    {"LEFTCLICK",    MOUSE_LEFT},
    {"LEFT_CLICK",   MOUSE_LEFT},
    {"RIGHTCLICK",   MOUSE_RIGHT},
    {"RIGHT_CLICK",  MOUSE_RIGHT},
    {"MIDDLECLICK",  MOUSE_MIDDLE},
    {"MIDDLE_CLICK", MOUSE_MIDDLE},
    {"WHEELCLICK",   MOUSE_MIDDLE},
    {"WHEEL_CLICK",  MOUSE_MIDDLE},
    {nullptr, 0}
};

/* ================================================================ */

namespace MOONCAKE::APPS
{

/* ================================================================ */
/*  Constructor / Lifecycle
 * ================================================================ */
AppBadUSB::AppBadUSB(DEVICES* device) : _device(device)
{
    setAppInfo().name = "Bad USB";
}

void AppBadUSB::onOpen()
{
    auto& Lcd = _device->Lcd;
    hp::drawChrome(Lcd);
    hp::drawHeader(Lcd, "Bad USB");
    hp::drawLoadingBegin(Lcd, "Loading...", "Scanning SD card");

    usb_manager_request(USB_MODE_HID);
    _scanScripts();
    _scanLayouts();
    _loadLayout(_layoutName.c_str());
    _switchPage(BuPage::FileList);
}

void AppBadUSB::onRunning()
{
    _device->button.update();
    _device->button.tick();

    if (_device->button.B.isLongPress()) {
        _stopExec();
        close();
        return;
    }

    if (_sceneDirty) {
        _sceneDirty = false;
        switch (_page) {
            case BuPage::FileList: _enterFileList(); break;
            case BuPage::Layouts:  _enterLayouts();  break;
            case BuPage::Running:  _enterRunning();  break;
        }
    }

    switch (_page) {
        case BuPage::FileList: _runFileList();    break;
        case BuPage::Layouts:  _runLayouts();     break;
        case BuPage::Running:  _updateRunning();  break;
    }
}

void AppBadUSB::onClose()
{
    _stopExec();
    usb_manager_release(USB_MODE_HID);
    if (_bleStarted) {
        bu_ble_end();
        _bleStarted = false;
    }
    _scriptFiles.clear();
    _lines.clear();
}

/* --- Connection helper: true if current mode's HID host is attached --- */
bool AppBadUSB::_isConnected() const
{
    return (_mode == BuMode::BLE) ? _bleConnected() : _hidConnected();
}

/* --- BLE keyboard helpers --- */
void AppBadUSB::_bleBegin()
{
    if (!_bleStarted) {
        _bleStarted = bu_ble_begin();
        Serial.println(_bleStarted ? "[BadUSB] BLE starting" : "[BadUSB] BLE unavailable; check Settings");
    }
}

bool AppBadUSB::_bleConnected() const
{
    return _bleStarted && bu_ble_connected();
}

/* --- Keyboard layout loader (Momentum .kl: 128 - uint16_le) --- */
bool AppBadUSB::_loadLayout(const char* name)
{
    _layoutLoaded = false;
    if (!name || !*name) return false;
    char path[64];
    snprintf(path, sizeof(path), "%s/%s.kl", LAYOUT_DIR, name);
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        Serial.printf("[BadUSB] Layout not found: %s\n", path);
        return false;
    }
    uint8_t buf[256];
    size_t n = f.read(buf, sizeof(buf));
    f.close();
    if (n != sizeof(buf)) {
        Serial.printf("[BadUSB] Layout %s: bad size %u\n", path, (unsigned)n);
        return false;
    }
    for (int i = 0; i < 128; i++) {
        _layout[i] = (uint16_t)buf[i * 2] | ((uint16_t)buf[i * 2 + 1] << 8);
    }
    _layoutName = name;
    _layoutLoaded = true;
    Serial.printf("[BadUSB] Layout loaded: %s\n", path);
    return true;
}

/* ================================================================ */
/*  Page management
 * ================================================================ */
void AppBadUSB::_switchPage(BuPage p)
{
    _page = p;
    _sceneDirty = true;
    _menuSel = 0;
    _scrollOffset = 0;
}

/* ================================================================ */
/*  TUI drawing helpers (identical to app_11)
 * ================================================================ */
void AppBadUSB::_drawHeader(const char* title)
{
    auto& Lcd = _device->Lcd;
    hp::drawChrome(Lcd);
    hp::drawHeader(Lcd, title);
}

void AppBadUSB::_drawMenuItem(int y, int index, const char* text, bool selected)
{
    (void)index;
    int row = (y - MENU_Y0) / ITEM_H;
    if (row < 0) row = 0;
    if (row >= hp::LIST_VIS) row = hp::LIST_VIS - 1;
    hp::drawListItem(_device->Lcd, row, text, selected);
    /* Erase the dotted bottom separator that drawListItem paints on
     * non-selected rows - we want a clean rectangular list. */
    if (!selected) {
        int sepY = hp::CON_Y0 + 2 + row * hp::ITEM_H + hp::ITEM_H - 1;
        int sepW = hp::W - 2 - hp::SBAR_W - 2;
        _device->Lcd.drawFastHLine(1, sepY, sepW, hp::COL_BG);
    }
    hp::drawScrollbar(_device->Lcd, _menuCount, _scrollOffset, MENU_VISIBLE);
}

/* All-bright 4-segment footer - every hint uses COL_FG instead of COL_DIM,
 * so [A]Enter / [B]Exit are easy to read on a CRT-style background. */
void AppBadUSB::_drawFooterBright4(const char* s1, const char* s2,
                                   const char* s3, const char* s4)
{
    auto& Lcd = _device->Lcd;
    Lcd.fillRect(1, hp::FTR_SEP + 1, hp::W - 2,
                 hp::FTR_BOTTOM - hp::FTR_SEP - 1, hp::COL_BG);
    Lcd.setFont(&fonts::efontCN_16);
    Lcd.setTextColor(hp::COL_FG, hp::COL_BG);
    if (s1 && s1[0]) { Lcd.setCursor(hp::PAD_X, hp::FTR_TXT); Lcd.print(s1); }
    if (s2 && s2[0]) { Lcd.setCursor(90,         hp::FTR_TXT); Lcd.print(s2); }
    if (s3 && s3[0]) { Lcd.setCursor(190,        hp::FTR_TXT); Lcd.print(s3); }
    if (s4 && s4[0]) {
        int tw = (int)strlen(s4) * 8;
        Lcd.setCursor(hp::W - tw - hp::PAD_X, hp::FTR_TXT);
        Lcd.print(s4);
    }
}

/* Three-segment footer with fixed slots: [Dir] left-aligned, [A] centred,
 * [B] right-aligned. Order is always Dir / A / B from left to right. */
void AppBadUSB::_drawFooterBright3(const char* dirHint, const char* aHint,
                                   const char* bHint)
{
    auto& Lcd = _device->Lcd;
    Lcd.fillRect(1, hp::FTR_SEP + 1, hp::W - 2,
                 hp::FTR_BOTTOM - hp::FTR_SEP - 1, hp::COL_BG);
    Lcd.setFont(&fonts::efontCN_16);
    Lcd.setTextColor(hp::COL_FG, hp::COL_BG);
    if (dirHint && dirHint[0]) {
        Lcd.setCursor(hp::PAD_X, hp::FTR_TXT);
        Lcd.print(dirHint);
    }
    if (aHint && aHint[0]) {
        int tw = (int)strlen(aHint) * 8;
        Lcd.setCursor((hp::W - tw) / 2, hp::FTR_TXT);
        Lcd.print(aHint);
    }
    if (bHint && bHint[0]) {
        int tw = (int)strlen(bHint) * 8;
        Lcd.setCursor(hp::W - tw - hp::PAD_X, hp::FTR_TXT);
        Lcd.print(bHint);
    }
}

void AppBadUSB::_drawFooter(const char* left, const char* right)
{
    hp::drawFooter(_device->Lcd, left, right);
}

void AppBadUSB::_drawMsgBox(const char* line1, const char* line2)
{
    hp::drawDialog(_device->Lcd, line1, line2);
}

/* ================================================================ */
/*  FileList -- browse /badusb/ for .txt scripts
 * ================================================================ */
void AppBadUSB::_enterFileList()
{
    _menuCount = (int)_scriptFiles.size();

    auto& Lcd = _device->Lcd;
    hp::drawChrome(Lcd);
    hp::drawHeader(Lcd, "Bad USB");
    const char* modeLabel = (_mode == BuMode::BLE) ? "[<>]BLE" : "[<>]USB";
    _drawFooterBright3(modeLabel, "[A]Enter", "[B]Exit");

    if (_menuCount == 0) {
        auto& Lcd = _device->Lcd;
        Lcd.setTextColor(BU_FG, BU_BG);
        Lcd.setCursor(20, 80);
        Lcd.print("No scripts found");
        Lcd.setCursor(20, 105);
        Lcd.printf("Place .txt in: %s/", SCRIPT_DIR);
    } else {
        int end = std::min(_menuCount - _scrollOffset, MENU_VISIBLE);
        for (int i = 0; i < end; i++) {
            const char* path = _scriptFiles[i + _scrollOffset].c_str();
            const char* slash = strrchr(path, '/');
            const char* fname = slash ? slash + 1 : path;
            _drawMenuItem(MENU_Y0 + i * ITEM_H, i, fname, i == _menuSel);
        }
    }
}

void AppBadUSB::_runFileList()
{
    /* [Left] / [Right] toggle USB ↔ BLE transport mode. */
    if (_device->button.Left.pressed() || _device->button.Right.pressed()) {
        _mode = (_mode == BuMode::USB) ? BuMode::BLE : BuMode::USB;
        if (_mode == BuMode::USB) { bu_ble_end(); _bleStarted=false; }
        const char* modeLabel = (_mode == BuMode::BLE) ? "[<>]BLE" : "[<>]USB";
        _drawFooterBright3(modeLabel, "[A]Enter", "[B]Exit");
        return;
    }

    if (_menuCount == 0) {
        if (_device->button.B.pressed()) close();
        return;
    }

    bool redraw = false;

    if (_device->button.Up.pressed()) {
        if (_menuSel > 0) { _menuSel--; }
        else if (_scrollOffset > 0) { _scrollOffset--; }
        redraw = true;
    }
    if (_device->button.Down.pressed()) {
        if (_menuSel < MENU_VISIBLE - 1 && _menuSel < _menuCount - _scrollOffset - 1) {
            _menuSel++;
        } else if (_scrollOffset + MENU_VISIBLE < _menuCount) {
            _scrollOffset++;
        }
        redraw = true;
    }

    if (redraw) {
        int visible = std::min(_menuCount - _scrollOffset, MENU_VISIBLE);
        for (int i = 0; i < MENU_VISIBLE; i++) {
            int dataIdx = i + _scrollOffset;
            if (i < visible) {
                const char* path = _scriptFiles[dataIdx].c_str();
                const char* slash = strrchr(path, '/');
                const char* fname = slash ? slash + 1 : path;
                _drawMenuItem(MENU_Y0 + i * ITEM_H, i, fname, i == _menuSel);
            } else {
                _device->Lcd.fillRect(0, MENU_Y0 + i * ITEM_H, SCR_W, ITEM_H, BU_BG);
            }
        }
    }

    if (_device->button.A.pressed()) {
        int idx = _menuSel + _scrollOffset;
        _selectedFile = _scriptFiles[idx];
        if (_loadScript(_selectedFile.c_str())) {
            /* Don't auto-execute \u2014 enter Running page in WaitConnect state.
             * Script only starts when USB is mounted and user confirms [A]. */
            _execState     = ExecState::WaitConnect;
            _execStartTime = 0;
            _execEndTime   = 0;
            _lineIdx       = 0;
            _switchPage(BuPage::Running);
        } else {
            const char* slash = strrchr(_selectedFile.c_str(), '/');
            _drawMsgBox("Failed to load!", slash ? slash + 1 : _selectedFile.c_str());
            delay(1500);
            _sceneDirty = true;
        }
    }
    if (_device->button.B.pressed()) {
        close();
    }
}

/* ================================================================ */
/*  Running Page  - partial-refresh implementation (no full-screen wipe)
 *
 *  Layout (320×240, content area y=26..212):
 *
 *    Header   y=0..24    filename + state badge
 *    Clock    y=30       MM:SS.CS  ui_font_number_60 (42 px line, full-width)
 *    Sep      y=76
 *    Icon     x=90, y=82 PNG 140×50 (connected / unconnected)
 *    Status   x=right, y=82  mode + connection label
 *    Pct      x=right, y=100 XX%  ui_font_number_24 (23 px)
 *    Counter  x=right, y=126 XX/YY efontCN_16 ×1
 *    Layout   y=138     centered dim label
 *    Bar      y=156..168 progress bar (height 12)
 *    Footer   y=215..239
 * ================================================================ */
void AppBadUSB::_enterRunning()
{
    /* Start BLE advertising if that mode is selected. */
    if (_mode == BuMode::BLE) {
        _bleBegin();
        if (!_bleStarted) {
            _errMsg=bluetooth_manager::enabled() ? "Bluetooth busy/error" : "Enable Bluetooth in Settings";
            _execState=ExecState::Error;
        }
    }

    /* Invalidate all dynamic caches so first dynamic pass repaints everything. */
    _lastClkMs      = 0xFFFFFFFFul;
    _lastPctInt     = -1;
    _lastLineIdx    = (size_t)-1;
    _lastDrawnState = ExecState::Idle;
    _lastDrawnMode  = (BuMode)0xFF;
    _lastConnected  = !_isConnected();  /* force repaint */
    _lastDraw       = 0;
    _bleReadyAt     = 0;
    _drawRunningStatic();
    _drawRunningDynamic();
}

/* Draw the static chrome + fixed labels once on page enter. */
void AppBadUSB::_drawRunningStatic()
{
    auto& Lcd = _device->Lcd;

    hp::drawChrome(Lcd);

    const char* fname = strrchr(_selectedFile.c_str(), '/');
    fname = fname ? fname + 1 : _selectedFile.c_str();
    hp::drawHeader(Lcd, fname);
    hp::clearContent(Lcd);

    /* Layout label — static, bottom of content zone */
    Lcd.setFont(&fonts::efontCN_16);
    Lcd.setTextColor(BU_FG, BU_BG);
    Lcd.setTextSize(1);
    char lblBuf[40];
    snprintf(lblBuf, sizeof(lblBuf), "layout: %s", _layoutName.c_str());
    int lblW = (int)strlen(lblBuf) * 8;
    Lcd.setCursor((SCR_W - lblW) / 2, 194);
    Lcd.print(lblBuf);
}

/* Draw the PNG connection-status icon (140×50) in the left half of the icon zone. */
void AppBadUSB::_drawConnIcon(bool connected)
{
    auto& Lcd = _device->Lcd;
    Lcd.fillRect(8, 108, 150, 52, BU_BG);
    if (connected)
        Lcd.drawPng(kIconConnected,   kIconConnectedLen,   8, 110);
    else
        Lcd.drawPng(kIconUnconnected, kIconUnconnectedLen, 8, 110);
}

/* State badge — top of content zone (y=26..58), wide pill, immediately visible.
 * Rendered into a sprite first so the push is atomic and flicker-free. */
void AppBadUSB::_drawStateBadge()
{
    auto& Lcd = _device->Lcd;
    const char* stStr = "";
    uint16_t    stCol = BU_FG;
    switch (_execState) {
    case ExecState::Idle:        stStr = "IDLE";   stCol = BU_FG;     break;
    case ExecState::WaitConnect: stStr = "WAIT";   stCol = 0xFFE0;    break;
    case ExecState::Ready:       stStr = "READY";  stCol = BU_ACCENT; break;
    case ExecState::Running:     stStr = "RUN";    stCol = BU_FG;     break;
    case ExecState::Paused:      stStr = "PAUSE";  stCol = 0xFFE0;    break;
    case ExecState::Delay:       stStr = "DELAY";  stCol = 0xFFE0;    break;
    case ExecState::StringDelay: stStr = "TYPE";   stCol = 0x07FF;    break;
    case ExecState::WaitButton:  stStr = "WAIT A"; stCol = 0xFD20;    break;
    case ExecState::Done:        stStr = "DONE";   stCol = BU_ACCENT; break;
    case ExecState::Error:       stStr = "ERROR";  stCol = TFT_RED;   break;
    }

    int tw = (int)strlen(stStr) * 8;
    int bw = (tw + 40 < 240) ? 240 : (tw + 40);
    int bx = (SCR_W - bw) / 2;
    uint16_t txt = (stCol == BU_FG || stCol == BU_ACCENT || stCol == 0xFFE0)
                   ? BU_BG : BU_ACCENT;

    /* Sprite spans the full badge row (y=26..58 = 32 px). */
    LGFX_Sprite spr(&Lcd);
    spr.setColorDepth(16);
    if (spr.createSprite(SCR_W - 2, 32)) {
        spr.fillSprite(BU_BG);
        /* sprite origin = screen (1, 26); badge top in sprite = 30-26 = 4 */
        int sbx = bx - 1;
        spr.fillRoundRect(sbx, 4, bw, 24, 4, stCol);
        spr.setFont(&fonts::efontCN_16);
        spr.setTextColor(txt, stCol);
        spr.setTextSize(1);
        spr.setCursor(sbx + (bw - tw) / 2, 6);
        spr.print(stStr);
        spr.pushSprite(1, 26);
        spr.deleteSprite();
    } else {
        Lcd.fillRect(1, 26, SCR_W - 2, 32, BU_BG);
        Lcd.fillRoundRect(bx, 30, bw, 24, 4, stCol);
        Lcd.setFont(&fonts::efontCN_16);
        Lcd.setTextColor(txt, stCol);
        Lcd.setTextSize(1);
        Lcd.setCursor(bx + (bw - tw) / 2, 32);
        Lcd.print(stStr);
    }
}

/* Footer hints depending on current state. Order is always [Dir]/[A]/[B].
 * [Dir]=Config (open Layouts), [A]=Run/Stop/Run-again, [B]=Back to list. */
void AppBadUSB::_drawRunningFooter()
{
    const char* a = "";
    switch (_execState) {
    case ExecState::WaitConnect: a = "[A]----"; break; /* not connected yet */
    case ExecState::Ready:       a = "[A]Run";  break;
    case ExecState::Running:
    case ExecState::Delay:
    case ExecState::StringDelay:
    case ExecState::WaitButton:
    case ExecState::Paused:      a = "[A]Stop"; break;
    case ExecState::Done:
    case ExecState::Error:       a = "[A]Run";  break; /* re-run from start */
    default:                     a = "[A]----"; break;
    }
    _drawFooterBright3("[^v]Config", a, "[B]Back");
}

/* Repaint only the values that have changed since the last call.
 * Caller invokes this each frame (~20-50 ms); cheap when nothing changed. */
void AppBadUSB::_drawRunningDynamic()
{
    auto& Lcd = _device->Lcd;

    /* --- Header badge (state changes) --- */
    if (_execState != _lastDrawnState) {
        _drawStateBadge();
        _drawRunningFooter();
        if (_execState == ExecState::Error) {
            Lcd.fillRect(2,192,316,20,BU_BG);
            Lcd.setFont(&fonts::efontCN_16);
            Lcd.setTextColor(TFT_RED,BU_BG);
            Lcd.setCursor(4,194);
            Lcd.printf("L%u: %.32s", unsigned(_errLine), _errMsg.c_str());
        } else if (_lastDrawnState == ExecState::Error) {
            // Restore the layout label after leaving ERROR; don't leave stale text.
            Lcd.fillRect(2,192,316,20,BU_BG);
            Lcd.setFont(&fonts::efontCN_16);
            Lcd.setTextColor(BU_FG,BU_BG);
            char label[40];
            snprintf(label, sizeof(label), "layout: %s", _layoutName.c_str());
            Lcd.setCursor((SCR_W - int(strlen(label)) * 8) / 2, 194);
            Lcd.print(label);
        }
        _lastDrawnState = _execState;
    }

    /* --- Connection icon (left) + mode/status text (right) --- */
    bool conn = _isConnected();
    if (conn != _lastConnected || _mode != _lastDrawnMode) {
        _drawConnIcon(conn);

        /* Right half of icon zone: x=158..316 (160px wide), y=108..162 */
        const int RH_X0 = 158, RH_W = SCR_W - RH_X0 - 4;
        Lcd.fillRect(RH_X0, 108, RH_W, 54, BU_BG);
        Lcd.setFont(&fonts::efontCN_16);
        Lcd.setTextSize(1);

        /* Mode badge (row 1) */
        const char* modeStr = (_mode == BuMode::BLE) ? "BLE" : "USB";
        int modeW = (int)strlen(modeStr) * 8;
        Lcd.setTextColor(BU_ACCENT, BU_BG);
        Lcd.setCursor(RH_X0 + (RH_W - modeW) / 2, 120);
        Lcd.print(modeStr);

        /* Connection status (row 2) */
        const char* connStr = conn ? "Connected" : "Waiting...";
        uint16_t connCol = conn ? BU_ACCENT : 0xFFE0;
        int connW = (int)strlen(connStr) * 8;
        Lcd.setTextColor(connCol, BU_BG);
        Lcd.setCursor(RH_X0 + (RH_W - connW) / 2, 140);
        Lcd.print(connStr);

        _lastConnected = conn;
        _lastDrawnMode = _mode;
    }

    /* --- Clock MM:SS.CS with ui_font_number_60 ---
     * lvglDrawStr draws pixel-by-pixel; sprite buffers the whole render
     * so pushSprite delivers it in one DMA blast — eliminates flicker. */
    unsigned long elapsed;
    if (_execStartTime == 0) {
        elapsed = 0;
    } else if (_execEndTime != 0) {
        elapsed = _execEndTime - _execStartTime;
    } else {
        elapsed = millis() - _execStartTime;
    }
    unsigned long elapsedTick = elapsed / 10;
    if (elapsedTick != _lastClkMs) {
        unsigned int mins = (elapsed / 60000) % 60;
        unsigned int secs = (elapsed /  1000) % 60;
        unsigned int cs   = (elapsed /    10) % 100;
        char clkBuf[12];
        snprintf(clkBuf, sizeof(clkBuf), "%02u:%02u.%02u", mins, secs, cs);

        LGFX_Sprite spr(&Lcd);
        spr.setColorDepth(16);
        if (spr.createSprite(SCR_W - 2, 46)) {
            spr.fillSprite(BU_BG);
            int32_t clkW = lvglStrW(&ui_font_number_60, clkBuf);
            /* center in sprite; sprite origin = screen (1, 58); text y=2 → screen y=60 */
            int clkSprX = ((SCR_W - 2) - (int)clkW) / 2;
            lvglDrawStr(spr, clkSprX, 2, &ui_font_number_60, clkBuf, BU_ACCENT);
            spr.pushSprite(1, 58);
            spr.deleteSprite();
        } else {
            Lcd.fillRect(1, 58, SCR_W - 2, 46, BU_BG);
            int32_t clkW = lvglStrW(&ui_font_number_60, clkBuf);
            int clkX = (SCR_W - (int)clkW) / 2;
            lvglDrawStr(Lcd, clkX, 60, &ui_font_number_60, clkBuf, BU_ACCENT);
        }
        _lastClkMs = elapsedTick;
    }

    /* --- Progress row: % (left half) + line counter (right half) ---
     * Both depend on _lineIdx so they always change together; update in one block. */
    float pct = _lines.empty() ? 0.0f
                               : (float)_lineIdx / (float)_lines.size();
    int pctInt = (int)(pct * 100.0f + 0.5f);
    if (pctInt != _lastPctInt || _lineIdx != _lastLineIdx) {
        Lcd.fillRect(1, 162, SCR_W - 2, 28, BU_BG);

        /* Left half (x=1..158): percentage with ui_font_number_24 */
        char numBuf[8];
        snprintf(numBuf, sizeof(numBuf), "%d", pctInt);
        int32_t numW = lvglStrW(&ui_font_number_24, numBuf);
        const int LH_W = 158;
        int pctX = 1 + (LH_W - (int)numW - 8) / 2;
        lvglDrawStr(Lcd, pctX, 164, &ui_font_number_24, numBuf, BU_FG);
        Lcd.setFont(&fonts::efontCN_16);
        Lcd.setTextColor(BU_FG, BU_BG);
        Lcd.setTextSize(1);
        Lcd.setCursor(pctX + (int)numW + 2, 171);
        Lcd.print('%');

        /* Right half (x=160..316): line counter */
        char cntBuf[16];
        snprintf(cntBuf, sizeof(cntBuf), "%d/%d",
                 (int)_lineIdx, (int)_lines.size());
        const int RH2_X0 = 160, RH2_W = SCR_W - RH2_X0 - 4;
        int cntW = (int)strlen(cntBuf) * 8;
        Lcd.setCursor(RH2_X0 + (RH2_W - cntW) / 2, 172);
        Lcd.print(cntBuf);

        _lastPctInt  = pctInt;
        _lastLineIdx = _lineIdx;
    }
}

/* ================================================================ */
/*  Running page input + step loop
 * ================================================================ */
void AppBadUSB::_updateRunning()
{
    if (_mode == BuMode::BLE) {
        const bool executing=_execState==ExecState::Running || _execState==ExecState::Delay ||
            _execState==ExecState::StringDelay || _execState==ExecState::WaitButton ||
            _execState==ExecState::Paused;
        if ((_bleStarted && bu_ble_failed()) || (executing && !_bleConnected())) {
            _stopExec();
            bu_ble_end(); _bleStarted=false;
            _errMsg="Bluetooth stopped. Reopen script.";
            _execState=ExecState::Error;
        }
    }
    if (_mode == BuMode::USB && !_isConnected()
        && (_execState == ExecState::Running || _execState == ExecState::Delay
            || _execState == ExecState::StringDelay || _execState == ExecState::WaitButton)) {
        _stopExec();
        _errMsg = "USB disconnected. Run again.";
        _errLine = _lineIdx;
        _execState = ExecState::Error;
    }
    /* --- State-independent inputs --- */
    /* Any direction key = enter Layouts (Config). If a script is actively
     * running, stop it first (releases all keys) so we don't leave the host
     * with stuck modifiers when the user reconfigures. */
    if (_device->button.Up.pressed()    || _device->button.Down.pressed() ||
        _device->button.Left.pressed()  || _device->button.Right.pressed()) {
        if (_execState == ExecState::Running     ||
            _execState == ExecState::Delay       ||
            _execState == ExecState::StringDelay ||
            _execState == ExecState::WaitButton  ||
            _execState == ExecState::Paused) {
            _kbdReleaseAll();
            _execState   = ExecState::Done;
            _execEndTime = millis();
        }
        _switchPage(BuPage::Layouts);
        return;
    }
    /* [B] short-press = back to script list (stops execution if needed). */
    if (_device->button.B.pressed()) {
        _stopExec();
        _switchPage(BuPage::FileList);
        return;
    }

    unsigned long now = millis();

    switch (_execState) {

    /* --- Waiting for host to mount HID (USB) or pair (BLE) --- */
    case ExecState::WaitConnect:
        if (_isConnected()) {
            // BLE readiness includes encryption and the host's HID subscription.
            _execState = ExecState::Ready;
        } else {
            _bleReadyAt = 0;   /* restart timer if connection drops */
        }
        if (now - _lastDraw > 200) { _drawRunningDynamic(); _lastDraw = now; }
        break;

    /* --- Connected, awaiting [A] to start --- */
    case ExecState::Ready:
        if (!_isConnected()) {
            _device->button.A.pressed(); // Consume a press after the host has gone away.
            _execState = ExecState::WaitConnect;
        } else if (_device->button.A.pressed()) {
            _startExec();
        }
        if (now - _lastDraw > 200) { _drawRunningDynamic(); _lastDraw = now; }
        break;

    /* --- Paused by user (legacy; [A] now stops instead) --- */
    case ExecState::Paused:
        if (_device->button.A.pressed()) {
            _kbdReleaseAll();
            _execState   = ExecState::Done;
            _execEndTime = millis();
        }
        if (now - _lastDraw > 200) { _drawRunningDynamic(); _lastDraw = now; }
        break;

    /* --- Finished / errored: [A] re-runs the same script from start --- */
    case ExecState::Idle:
    case ExecState::Done:
    case ExecState::Error:
        if (_execEndTime == 0 &&
            (_execState == ExecState::Done || _execState == ExecState::Error))
            _execEndTime = millis();
        if (_device->button.A.pressed()) {
            if (_isConnected()) _startExec();
            else                _execState = ExecState::WaitConnect;
        }
        if (now - _lastDraw > 200) { _drawRunningDynamic(); _lastDraw = now; }
        break;

    case ExecState::WaitButton:
        /* [A] = stop & abort (no pause feature per spec). */
        if (_device->button.A.pressed()) {
            _kbdReleaseAll();
            _execState   = ExecState::Done;
            _execEndTime = millis();
        }
        if (now - _lastDraw > 200) { _drawRunningDynamic(); _lastDraw = now; }
        break;

    case ExecState::Delay:
        if (_device->button.A.pressed()) {
            _kbdReleaseAll();
            _execState   = ExecState::Done;
            _execEndTime = millis();
            break;
        }
        if (badusb_parse::deadlineReached(now, _delayEnd)) _execState = ExecState::Running;
        if (now - _lastDraw > 100) { _drawRunningDynamic(); _lastDraw = now; }
        break;

    case ExecState::StringDelay:
        if (_device->button.A.pressed()) {
            _kbdReleaseAll();
            _execState   = ExecState::Done;
            _execEndTime = millis();
            break;
        }
        if (badusb_parse::deadlineReached(now, _delayEnd) && _printPos < _printStr.size()) {
            char c = _printStr[_printPos];
            if (c != '\n') {
                _kbdWrite((uint8_t)c);
            } else {
                _kbdPress(KEY_RETURN); delay(2); _kbdRelease(KEY_RETURN);
            }
            _printPos++;
            _delayEnd = now + (_strDelay > 0 ? _strDelay : _defStrDelay);
        }
        if (_printPos >= _printStr.size()) {
            _strDelay = 0;
            if (_defDelay > 0) {
                _delayEnd  = millis() + _defDelay;
                _execState = ExecState::Delay;
            } else {
                _execState = ExecState::Running;
            }
        }
        if (now - _lastDraw > 100) { _drawRunningDynamic(); _lastDraw = now; }
        break;

    case ExecState::Running: {
        /* [A] = stop & abort (afterwards [A] becomes Run again to re-run). */
        if (_device->button.A.pressed()) {
            _kbdReleaseAll();
            _execState   = ExecState::Done;
            _execEndTime = millis();
            break;
        }
        int batch = 0;
        while (_execState == ExecState::Running && batch < 20) {
            batch++;

            const char* lineStr = nullptr;
            bool fromRepeat = false;
            if (_repeatCnt > 0) {
                if (_repeatLine.empty()) { _repeatCnt = 0; continue; }
                lineStr = _repeatLine.c_str();
                _repeatCnt--;
                fromRepeat = true;
            } else {
                if (_lineIdx >= _lines.size()) {
                    _execState   = ExecState::Done;
                    _execEndTime = millis();
                    _kbdReleaseAll();
                    break;
                }
                _prevLine = _lines[_lineIdx];
                lineStr   = _prevLine.c_str();
                _lineIdx++;
            }

            int32_t r = _parseLine(lineStr);
            if (_mode == BuMode::BLE && (!_bleConnected() || bu_ble_failed())) {
                _stopExec(); bu_ble_end(); _bleStarted=false;
                _errMsg="Bluetooth stopped. Reopen script.";
                _execState=ExecState::Error;
                break;
            }

            if (!fromRepeat && r != -1 && r > -10) {
                const char* trimmed = _skipWs(lineStr);
                if (strncmp(trimmed, "REPEAT", 6) != 0)
                    _repeatLine = trimmed;
            }

            if (r > 0) {
                _delayEnd  = millis() + badusb_parse::delaySum(uint32_t(r), _defDelay);
                _execState = ExecState::Delay;
            } else if (r == -2) {
                _execState = ExecState::WaitButton;
            } else if (r == -5) {
                _execState = ExecState::StringDelay;
            } else if (r <= -10) {
                _errLine = _lineIdx;
                if (_errMsg.empty()) _errMsg = "Parse error";
                _execState   = ExecState::Error;
                _execEndTime = millis();
                _kbdReleaseAll();
            } else {
                if (r == 0 && _defDelay > 0 && _repeatCnt == 0) {
                    _delayEnd  = millis() + _defDelay;
                    _execState = ExecState::Delay;
                }
            }
        }
        if (now - _lastDraw > 50) { _drawRunningDynamic(); _lastDraw = now; }
        break;
    }
    }
}

/* ================================================================ */
/*  SD card -- scan /badusb/ for scripts
 * ================================================================ */
void AppBadUSB::_scanScripts()
{
    _scriptFiles.clear();

    if (!SD_MMC.exists(SCRIPT_DIR)) {
        SD_MMC.mkdir(SCRIPT_DIR);
    }

    File dir = SD_MMC.open(SCRIPT_DIR);
    if (!dir || !dir.isDirectory()) {
        Serial.printf("[BadUSB] Failed to open %s\n", SCRIPT_DIR);
        return;
    }

    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) {
            String name = entry.name();
            if (name.endsWith(".txt") || name.endsWith(".TXT"))
                _scriptFiles.push_back(std::string(entry.path()));
        }
        entry.close();
    }
    dir.close();

    std::sort(_scriptFiles.begin(), _scriptFiles.end());
    Serial.printf("[BadUSB] Found %d script(s)\n", (int)_scriptFiles.size());
}

/* ================================================================ */
/*  Layouts page - pick a .kl file from /badusb/layouts
 * ================================================================ */
void AppBadUSB::_scanLayouts()
{
    _layoutFiles.clear();

    if (!SD_MMC.exists(LAYOUT_DIR)) {
        SD_MMC.mkdir(LAYOUT_DIR);
    }
    File dir = SD_MMC.open(LAYOUT_DIR);
    if (!dir || !dir.isDirectory()) {
        Serial.printf("[BadUSB] Failed to open %s\n", LAYOUT_DIR);
        return;
    }
    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) {
            String name = entry.name();
            if (name.endsWith(".kl") || name.endsWith(".KL"))
                _layoutFiles.push_back(std::string(entry.path()));
        }
        entry.close();
    }
    dir.close();
    std::sort(_layoutFiles.begin(), _layoutFiles.end());
    Serial.printf("[BadUSB] Found %d layout(s)\n", (int)_layoutFiles.size());
}

/* Helper: extract bare layout name from "/badusb/layouts/en-US.kl" - "en-US". */
static std::string _layoutBasename(const std::string& path)
{
    const char* p = path.c_str();
    const char* slash = strrchr(p, '/');
    std::string base = slash ? std::string(slash + 1) : path;
    auto dot = base.rfind('.');
    if (dot != std::string::npos) base.resize(dot);
    return base;
}

void AppBadUSB::_enterLayouts()
{
    _menuCount = (int)_layoutFiles.size();

    auto& Lcd = _device->Lcd;
    hp::drawChrome(Lcd);
    hp::drawHeader(Lcd, "Keyboard Layout");
    _drawFooterBright3("[^v]Select", "[A]Apply", "[B]Back");

    if (_menuCount == 0) {
        Lcd.setTextColor(BU_FG, BU_BG);
        Lcd.setCursor(20, 80);
        Lcd.print("No layouts found");
        Lcd.setCursor(20, 105);
        Lcd.printf("Place .kl in: %s/", LAYOUT_DIR);
        return;
    }

    int end = std::min(_menuCount - _scrollOffset, MENU_VISIBLE);
    for (int i = 0; i < end; i++) {
        std::string name = _layoutBasename(_layoutFiles[i + _scrollOffset]);
        /* Mark the currently active layout with a leading '*'. */
        if (_layoutLoaded && name == _layoutName) {
            std::string marked = std::string("* ") + name;
            _drawMenuItem(MENU_Y0 + i * ITEM_H, i, marked.c_str(),
                          i == _menuSel);
        } else {
            _drawMenuItem(MENU_Y0 + i * ITEM_H, i, name.c_str(),
                          i == _menuSel);
        }
    }
}

void AppBadUSB::_runLayouts()
{
    if (_menuCount == 0) {
        if (_device->button.B.pressed()) {
            /* Return to whichever page invoked Layouts: FileList by default,
             * but if a script is loaded we came from Running. */
            _switchPage(_lines.empty() ? BuPage::FileList : BuPage::Running);
        }
        return;
    }

    bool redraw = false;

    if (_device->button.Up.pressed()) {
        if (_menuSel > 0) { _menuSel--; }
        else if (_scrollOffset > 0) { _scrollOffset--; }
        redraw = true;
    }
    if (_device->button.Down.pressed()) {
        if (_menuSel < MENU_VISIBLE - 1 && _menuSel < _menuCount - _scrollOffset - 1) {
            _menuSel++;
        } else if (_scrollOffset + MENU_VISIBLE < _menuCount) {
            _scrollOffset++;
        }
        redraw = true;
    }

    if (redraw) {
        int visible = std::min(_menuCount - _scrollOffset, MENU_VISIBLE);
        for (int i = 0; i < MENU_VISIBLE; i++) {
            int dataIdx = i + _scrollOffset;
            if (i < visible) {
                std::string name = _layoutBasename(_layoutFiles[dataIdx]);
                if (_layoutLoaded && name == _layoutName) {
                    std::string marked = std::string("* ") + name;
                    _drawMenuItem(MENU_Y0 + i * ITEM_H, i, marked.c_str(),
                                  i == _menuSel);
                } else {
                    _drawMenuItem(MENU_Y0 + i * ITEM_H, i, name.c_str(),
                                  i == _menuSel);
                }
            } else {
                _device->Lcd.fillRect(0, MENU_Y0 + i * ITEM_H, SCR_W, ITEM_H, BU_BG);
            }
        }
    }

    /* [A] = apply selected layout. */
    if (_device->button.A.pressed()) {
        int idx = _menuSel + _scrollOffset;
        std::string name = _layoutBasename(_layoutFiles[idx]);
        if (_loadLayout(name.c_str())) {
            _drawMsgBox("Layout applied:", name.c_str());
            delay(700);
        } else {
            _drawMsgBox("Load failed:", name.c_str());
            delay(900);
        }
        _switchPage(_lines.empty() ? BuPage::FileList : BuPage::Running);
        return;
    }
    /* [B] = back without applying. */
    if (_device->button.B.pressed()) {
        _switchPage(_lines.empty() ? BuPage::FileList : BuPage::Running);
        return;
    }
}

/* ================================================================ */
/*  Script control
 * ================================================================ */
bool AppBadUSB::_loadScript(const char* path)
{
    _lines.clear();
    _lineIdx    = 0;
    _repeatCnt  = 0;
    _defDelay   = 0;
    _strDelay   = 0;
    _defStrDelay = 0;
    _keyHoldNb  = 0;
    _errMsg.clear();
    _errLine    = 0;

    File f = SD_MMC.open(path);
    if (!f) { _errMsg = "Cannot open file"; return false; }

    while (f.available()) {
        String ln = f.readStringUntil('\n');
        if (_lines.empty() && ln.startsWith("\xEF\xBB\xBF")) ln.remove(0, 3);
        while (ln.length() > 0) {
            char c = ln.charAt(ln.length() - 1);
            if (c == '\r' || c == ' ' || c == '\t')
                ln.remove(ln.length() - 1);
            else break;
        }
        _lines.push_back(std::string(ln.c_str()));
        if (_lines.size() > 4000) {
            _errMsg = "Script too large (>4000 lines)";
            f.close(); _lines.clear();
            return false;
        }
    }
    f.close();
    Serial.printf("[BadUSB] Loaded %d lines from %s\n",
                  (int)_lines.size(), path);

    /* ================================================================ DEFINE pre-processor (Momentum) ================================================================
     * "DEFINE NAME value" lines are stripped; remaining occurrences of NAME
     * are replaced with value in subsequent lines (simple substring sub). */
    {
        std::vector<std::pair<std::string,std::string>> macros;
        for (auto& ln : _lines) {
            const char* s = ln.c_str();
            while (*s == ' ' || *s == '\t') s++;
            if (strncmp(s, "DEFINE", 6) != 0 || (s[6] != ' ' && s[6] != '\t')) {
                /* not a DEFINE line - apply known macros (longest name first
                 * so prefixes never shadow longer names). */
                for (auto& m : macros) {
                    size_t pos = 0;
                    while ((pos = ln.find(m.first, pos)) != std::string::npos) {
                        ln.replace(pos, m.first.size(), m.second);
                        pos += m.second.size();
                    }
                }
                continue;
            }
            const char* p = s + 6;
            while (*p == ' ' || *p == '\t') p++;
            const char* nameStart = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            std::string name(nameStart, p - nameStart);
            while (*p == ' ' || *p == '\t') p++;
            std::string value(p);   /* rest of line */
            if (!name.empty()) {
                /* keep macros sorted longest-first for greedy substitution */
                auto it = macros.begin();
                while (it != macros.end() && it->first.size() >= name.size()) ++it;
                macros.insert(it, {name, value});
            }
            ln.clear();   /* strip the DEFINE line itself */
        }
    }
    return true;
}

void AppBadUSB::_startExec()
{
    _lineIdx    = 0;
    _repeatCnt  = 0;
    _defDelay   = 0;
    _strDelay   = 0;
    _defStrDelay = 0;
    _keyHoldNb  = 0;
    _printStr.clear();
    _printPos   = 0;
    _repeatLine.clear();
    _prevLine.clear();
    _errMsg.clear();
    _errLine    = 0;
    _execStartTime = millis();
    _execEndTime   = 0;
    _execState  = ExecState::Running;
    _lastDraw   = 0;
    _kbdReleaseAll();
}

void AppBadUSB::_stopExec()
{
    _execState   = ExecState::Idle;
    _execEndTime = 0;
    _kbdReleaseAll();
}

/* ================================================================ */
/*  DuckyScript Parser helpers
 * ================================================================ */
const char* AppBadUSB::_skipWs(const char* p)
{
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

bool AppBadUSB::_isEnd(char c)
{
    return c == ' ' || c == '\t' || c == '\0' || c == '\r' || c == '\n';
}

uint32_t AppBadUSB::_cmdLen(const char* line)
{
    return badusb_parse::commandLength(line);
}

bool AppBadUSB::_parseUint(const char* s, uint32_t& v)
{
    uint32_t parsed;
    if (!badusb_parse::unsignedValue(s, parsed) || parsed > uint32_t(INT32_MAX)) return false;
    v = parsed;
    return true;
}

bool AppBadUSB::_parseInt(const char* s, int32_t& v)
{
    return badusb_parse::signedValue(s, v);
}

uint8_t AppBadUSB::_nextMod(const char** p)
{
    const char* in = *p;
    for (const KEntry* m = s_mods; m->name; m++) {
        size_t mlen = strlen(m->name);
        if (strncasecmp(in, m->name, mlen) == 0) {
            char nxt = in[mlen];
            if (_isEnd(nxt) || nxt == '-') {
                *p = in + mlen;
                return m->code;
            }
        }
    }
    return 0;
}

uint8_t AppBadUSB::_keyByName(const char* name)
{
    for (const KEntry* e = s_keys; e->name; e++) {
        size_t kl = strlen(e->name);
        if (strncasecmp(name, e->name, kl) == 0 && _isEnd(name[kl]))
            return e->code;
    }
    return 0;
}

uint8_t AppBadUSB::_modByName(const char* name)
{
    for (const KEntry* e = s_mods; e->name; e++) {
        size_t kl = strlen(e->name);
        if (strncasecmp(name, e->name, kl) == 0 && _isEnd(name[kl]))
            return e->code;
    }
    return 0;
}

uint16_t AppBadUSB::_mediaByName(const char* name)
{
    for (const MEntry* e = s_media; e->name; e++) {
        size_t kl = strlen(e->name);
        if (strncasecmp(name, e->name, kl) == 0 && _isEnd(name[kl]))
            return e->code;
    }
    return 0;
}

uint8_t AppBadUSB::_mouseByName(const char* name)
{
    for (const KEntry* e = s_mouseKeys; e->name; e++) {
        size_t kl = strlen(e->name);
        if (strncasecmp(name, e->name, kl) == 0 && _isEnd(name[kl]))
            return e->code;
    }
    return 0;
}

/* ================================================================ */
/*  _parseLine -- core DuckyScript command dispatcher
 *
 *  Returns:
 *    >0  delay in ms
 *     0  command executed OK, continue
 *    -1  skip / nop (comment, empty line)
 *    -2  WAIT_FOR_BUTTON_PRESS
 *    -5  string-with-delay started  (StringDelay state)
 *   -10  error
 * ================================================================ */
int32_t AppBadUSB::_parseLine(const char* line)
{
    line = _skipWs(line);

    if (*line == '\0' || *line == '\n' || *line == '\r') return -1;
    if (strncmp(line, "REM", 3) == 0 &&
        (line[3] == ' ' || line[3] == '\t' || line[3] == '\0'))
        return -1;

    if (strncmp(line, "ID", 2) == 0 && _isEnd(line[2]))  return -1;
    if (strncmp(line, "ID ", 3) == 0)                     return -1;
    if (strncmp(line, "BT_ID", 5) == 0)                   return -1;
    if (strncmp(line, "BLE_ID", 6) == 0)                  return -1;

    uint32_t cl = _cmdLen(line);
    const char* param = badusb_parse::arguments(line, cl);

    /* --- DELAY n --- */
    if (cl == 5 && strncmp(line, "DELAY", 5) == 0) {
        uint32_t ms = 0;
        if (_parseUint(param, ms)) return (int32_t)ms;
        _errMsg = "DELAY: bad number";
        return -10;
    }

    /* --- DEFAULT_DELAY / DEFAULTDELAY --- */
    if ((cl == 13 && strncmp(line, "DEFAULT_DELAY", 13) == 0) ||
        (cl == 12 && strncmp(line, "DEFAULTDELAY", 12) == 0)) {
        if (_parseUint(param, _defDelay)) return 0;
        _errMsg = "DEFAULTDELAY: bad number";
        return -10;
    }

    /* --- STRING_DELAY / STRINGDELAY --- */
    if ((cl == 12 && strncmp(line, "STRING_DELAY", 12) == 0) ||
        (cl == 11 && strncmp(line, "STRINGDELAY", 11) == 0)) {
        if (_parseUint(param, _strDelay)) return 0;
        _errMsg = "STRINGDELAY: bad number";
        return -10;
    }

    /* --- DEFAULT_STRING_DELAY / DEFAULTSTRINGDELAY --- */
    if ((cl == 20 && strncmp(line, "DEFAULT_STRING_DELAY", 20) == 0) ||
        (cl == 19 && strncmp(line, "DEFAULTSTRINGDELAY", 19) == 0)) {
        if (_parseUint(param, _defStrDelay)) return 0;
        _errMsg = "DEFSTRINGDELAY: bad number";
        return -10;
    }

    /* --- STRINGLN text --- */
    if (cl == 8 && strncmp(line, "STRINGLN", 8) == 0) {
        const char* text = param;
        uint32_t sd = _strDelay > 0 ? _strDelay : _defStrDelay;
        if (sd > 0) {
            _printStr  = text;
            _printStr += '\n';
            _printPos  = 0;
            _delayEnd  = millis();
            return -5;
        }
        if (*text) _kbdPrint(text);
        _kbdPress(KEY_RETURN); delay(2); _kbdRelease(KEY_RETURN);
        return 0;
    }

    /* --- STRING text --- */
    if (cl == 6 && strncmp(line, "STRING", 6) == 0) {
        const char* text = (strlen(line) > 7) ? param : "";
        uint32_t sd = _strDelay > 0 ? _strDelay : _defStrDelay;
        if (sd > 0) {
            _printStr = text;
            _printPos = 0;
            _delayEnd = millis();
            return -5;
        }
        if (*text) _kbdPrint(text);
        return 0;
    }

    /* --- REPEAT n --- */
    if (cl == 6 && strncmp(line, "REPEAT", 6) == 0) {
        if (_parseUint(param, _repeatCnt) && _repeatCnt > 0) return 0;
        _errMsg = "REPEAT: bad number";
        return -10;
    }

    /* --- SYSRQ key --- */
    if (cl == 5 && strncmp(line, "SYSRQ", 5) == 0) {
        const char* kn = _skipWs(param);
        if (!_keyByName(kn) && !(strlen(kn) == 1 && *kn >= 0x20 && *kn <= 0x7E)) {
            _errMsg = "SYSRQ: missing/unknown key"; return -10;
        }
        _kbdPress(KEY_LEFT_ALT);
        _kbdPress(KEY_PRTSCR);
        uint8_t k = _keyByName(kn);
        if (k) _kbdPress(k);
        else if (strlen(kn) == 1) _kbdPress((uint8_t)kn[0]);
        delay(10);
        _kbdReleaseAll();
        return 0;
    }

    /* --- ALTCHAR charcode --- */
    if (cl == 7 && strncmp(line, "ALTCHAR", 7) == 0) {
        uint32_t code;
        if (!badusb_parse::unsignedValue(param, code)) {
            _errMsg = "ALTCHAR: bad number"; return -10;
        }
        _numlockOn();
        _altChar(_skipWs(param));
        return 0;
    }

    /* --- ALTSTRING / ALTCODE text --- */
    if ((cl == 9 && strncmp(line, "ALTSTRING", 9) == 0) ||
        (cl == 7 && strncmp(line, "ALTCODE", 7) == 0)) {
        _numlockOn();
        _altString(_skipWs(param));
        return 0;
    }

    /* --- HOLD key --- */
    if (cl == 4 && strncmp(line, "HOLD", 4) == 0) {
        if (_keyHoldNb >= 5) {
            _errMsg = "Too many keys held (max 5)";
            return -10;
        }
        const char* kn = (strlen(line) > 5) ? _skipWs(line + 5) : "";
        if (!*kn) { _errMsg = "HOLD: missing key"; return -10; }
        uint8_t mb = _mouseByName(kn);
        if (mb) { _keyHoldNb++; _mousePress(mb); return 0; }
        uint8_t mod = _modByName(kn);
        if (mod) { _keyHoldNb++; _kbdPress(mod); return 0; }
        uint8_t k = _keyByName(kn);
        if (k) { _keyHoldNb++; _kbdPress(k); return 0; }
        if (strlen(kn) == 1) { _keyHoldNb++; _kbdPress((uint8_t)kn[0]); return 0; }
        _errMsg = "HOLD: unknown key"; return -10;
    }

    /* --- RELEASE key --- */
    if (cl == 7 && strncmp(line, "RELEASE", 7) == 0) {
        if (_keyHoldNb == 0) { _errMsg = "No keys held"; return -10; }
        const char* kn = (strlen(line) > 8) ? _skipWs(param) : "";
        if (!*kn) { _errMsg = "RELEASE: missing key"; return -10; }
        uint8_t mb = _mouseByName(kn);
        if (mb) { _keyHoldNb--; _mouseRelease(mb); return 0; }
        uint8_t mod = _modByName(kn);
        if (mod) { _keyHoldNb--; _kbdRelease(mod); return 0; }
        uint8_t k = _keyByName(kn);
        if (k) { _keyHoldNb--; _kbdRelease(k); return 0; }
        if (strlen(kn) == 1) { _keyHoldNb--; _kbdRelease((uint8_t)kn[0]); return 0; }
        _errMsg = "RELEASE: unknown key"; return -10;
    }

    /* --- WAIT_FOR_BUTTON_PRESS --- */
    if (cl == 21 && strncmp(line, "WAIT_FOR_BUTTON_PRESS", 21) == 0 && !*param) return -2;

    /* --- MEDIA key --- */
    if (cl == 5 && strncmp(line, "MEDIA", 5) == 0) {
        const char* kn = _skipWs(param);
        uint16_t mc = _mediaByName(kn);
        if (mc) { _consumerPress(mc); _consumerRelease(); return 0; }
        _errMsg = "MEDIA: unknown key"; return -10;
    }

    /* --- GLOBE key (macOS Fn/Globe + key) --- */
    if (cl == 5 && strncmp(line, "GLOBE", 5) == 0) {
        const char* kn = _skipWs(param);
        uint8_t k = _keyByName(kn);
        if (!k && strlen(kn) == 1) k = (uint8_t)kn[0];
        if (!k) { _errMsg = "GLOBE: unknown key"; return -10; }
        _consumerPress(CC_FN_GLOBE);
        _kbdPress(k); _kbdRelease(k);
        _consumerRelease();
        return 0;
    }

    /* --- MOUSEMOVE / MOUSE_MOVE x y --- */
    if ((cl == 9 && strncmp(line, "MOUSEMOVE", 9) == 0) ||
        (cl == 10 && strncmp(line, "MOUSE_MOVE", 10) == 0)) {
        const char* args = _skipWs(param);
        int32_t mx = 0, my = 0;
        if (!badusb_parse::signedToken(args, mx)) { _errMsg = "MOUSEMOVE: bad X"; return -10; }
        if (!badusb_parse::signedValue(args, my)) { _errMsg = "MOUSEMOVE: bad Y"; return -10; }
        if (mx < -127 || mx > 127 || my < -127 || my > 127) {
            _errMsg = "MOUSEMOVE: range -127..127"; return -10;
        }
        _mouseMove((int8_t)mx, (int8_t)my);
        return 0;
    }

    /* --- MOUSESCROLL / MOUSE_SCROLL n --- */
    if ((cl == 11 && strncmp(line, "MOUSESCROLL", 11) == 0) ||
        (cl == 12 && strncmp(line, "MOUSE_SCROLL", 12) == 0)) {
        int32_t s = 0;
        if (!_parseInt(param, s)) { _errMsg = "MOUSESCROLL: bad N"; return -10; }
        if (s < -127 || s > 127) { _errMsg = "MOUSESCROLL: range -127..127"; return -10; }
        _mouseScroll((int8_t)s);
        return 0;
    }

    /* Standalone mouse click */
    {
        uint8_t mb = _mouseByName(line);
        if (mb) { _mouseClick(mb); return 0; }
    }

    /* Modifier chain + key:  CTRL-ALT DELETE,  GUI r, etc. */
    {
        const char* p = line;
        uint8_t mods[8];
        int nMod = 0;
        bool found = true;
        while (found && nMod < 8) {
            found = false;
            p = _skipWs(p);
            if (*p == '\0') break;
            uint8_t m = _nextMod(&p);
            if (m) {
                mods[nMod++] = m;
                if (*p == '-' || *p == ' ' || *p == '\t') p++;
                found = true;
            }
        }

        if (nMod > 0) {
            p = _skipWs(p);
            uint8_t k = 0;
            if (*p) {
                k = _keyByName(p);
                if (!k && strlen(p) == 1 && *p >= 0x20 && *p <= 0x7E) k = uint8_t(*p);
                if (!k) { _errMsg = "Unknown combo key"; return -10; }
            }
            for (int i = 0; i < nMod; i++) _kbdPress(mods[i]);
            if (k) _kbdPress(k);
            delay(10);
            _kbdReleaseAll();
            return 0;
        }

        p = _skipWs(line);
        uint8_t k = _keyByName(p);
        if (k) {
            _kbdPress(k); delay(10); _kbdRelease(k);
            return 0;
        }

        if (strlen(p) == 1 && *p >= 0x20 && *p <= 0x7E) {
            _kbdPress((uint8_t)*p); delay(10); _kbdRelease((uint8_t)*p);
            return 0;
        }
    }

    Serial.printf("[BadUSB] Unknown cmd: %s\n", line);
    _errMsg = "Unknown command";
    return -10;
}

/* ================================================================ */
/*  HID wrappers -- all no-ops when ARDUINO_USB_MODE != 0
 * ================================================================ */
void AppBadUSB::_kbdPress(uint8_t k)
{
    if (_mode == BuMode::BLE) { bu_ble_press(k); return; }
#if ARDUINO_USB_MODE == 0
    usb_hid_kbd().press(k);
#else
    (void)k;
#endif
}

void AppBadUSB::_kbdRelease(uint8_t k)
{
    if (_mode == BuMode::BLE) { bu_ble_release(k); return; }
#if ARDUINO_USB_MODE == 0
    usb_hid_kbd().release(k);
#else
    (void)k;
#endif
}

void AppBadUSB::_kbdReleaseAll()
{
    _keyHoldNb = 0;
    if (_mode == BuMode::BLE) { bu_ble_release_all(); return; }
#if ARDUINO_USB_MODE == 0
    usb_hid_kbd().releaseAll();
    usb_hid_mouse().release(0x07);
    usb_hid_consumer().release();
#endif
}

void AppBadUSB::_kbdPrint(const char* s)
{
    if (_mode == BuMode::BLE) { bu_ble_print(s); return; }
#if ARDUINO_USB_MODE == 0
    /* Honour active keyboard layout (USB only - BLE has no pressRaw). */
    if (_layoutLoaded) {
        for (const char* p = s; p && *p; ++p) _kbdWrite((uint8_t)*p);
        return;
    }
    usb_hid_kbd().print(s);
#else
    (void)s;
#endif
}

void AppBadUSB::_kbdWrite(uint8_t c)
{
    if (_mode == BuMode::BLE) { bu_ble_write(c); return; }
#if ARDUINO_USB_MODE == 0
    /* Apply keyboard layout when loaded (USB only). */
    if (_layoutLoaded && c < 128) {
        uint16_t entry = _layout[c];
        uint8_t  hid   = entry & 0xFF;
        uint8_t  mod   = entry >> 8;
        if (hid != 0) {
            if (mod & 0x01) usb_hid_kbd().press(KEY_LEFT_CTRL);
            if (mod & 0x02) usb_hid_kbd().press(KEY_LEFT_SHIFT);
            if (mod & 0x04) usb_hid_kbd().press(KEY_LEFT_ALT);
            if (mod & 0x08) usb_hid_kbd().press(KEY_LEFT_GUI);
            if (mod & 0x40) usb_hid_kbd().press(KEY_RIGHT_ALT);
            usb_hid_kbd().pressRaw(hid);
            delay(2);
            usb_hid_kbd().releaseRaw(hid);
            usb_hid_kbd().releaseAll();
            return;
        }
    }
    usb_hid_kbd().write(c);
#else
    (void)c;
#endif
}

void AppBadUSB::_mouseClick(uint8_t b)
{
    if (_mode == BuMode::BLE) { (void)b; return; }   /* BleKeyboard has no mouse */
#if ARDUINO_USB_MODE == 0
    usb_hid_mouse().click(b);
#else
    (void)b;
#endif
}

void AppBadUSB::_mousePress(uint8_t b)
{
    if (_mode == BuMode::BLE) { (void)b; return; }
#if ARDUINO_USB_MODE == 0
    usb_hid_mouse().press(b);
#else
    (void)b;
#endif
}

void AppBadUSB::_mouseRelease(uint8_t b)
{
    if (_mode == BuMode::BLE) { (void)b; return; }
#if ARDUINO_USB_MODE == 0
    usb_hid_mouse().release(b);
#else
    (void)b;
#endif
}

void AppBadUSB::_mouseMove(int8_t x, int8_t y)
{
    if (_mode == BuMode::BLE) {
        /* BleKeyboard does not implement mouse movement. */
        (void)x; (void)y;
        return;
    }
#if ARDUINO_USB_MODE == 0
    usb_hid_mouse().move(x, y, 0, 0);
#else
    (void)x; (void)y;
#endif
}

void AppBadUSB::_mouseScroll(int8_t scroll)
{
    if (_mode == BuMode::BLE) {
        (void)scroll;
        return;
    }
#if ARDUINO_USB_MODE == 0
    usb_hid_mouse().move(0, 0, scroll, 0);
#else
    (void)scroll;
#endif
}

void AppBadUSB::_consumerPress(uint16_t k)
{
    if (_mode == BuMode::BLE) {
        /* BleKeyboard has its own media-key constants and a press() overload
         * for MediaKeyReport; raw 16-bit consumer codes aren't supported here. */
        (void)k;
        return;
    }
#if ARDUINO_USB_MODE == 0
    usb_hid_consumer().press(k);
#else
    (void)k;
#endif
}

void AppBadUSB::_consumerRelease()
{
    if (_mode == BuMode::BLE) return;
#if ARDUINO_USB_MODE == 0
    usb_hid_consumer().release();
#endif
}

/* ================================================================ */
/*  ALTCHAR / ALTSTRING -- Alt + numpad character entry
 * ================================================================ */
void AppBadUSB::_numlockOn()
{
#if ARDUINO_USB_MODE == 0
    if (!(usb_hid_leds() & LED_NUMLOCK)) {
        usb_hid_kbd().press(KEY_NUM_LOCK);
        delay(5);
        usb_hid_kbd().release(KEY_NUM_LOCK);
        delay(50);
    }
#endif
}

bool AppBadUSB::_numpadPress(char digit)
{
    if (digit < '0' || digit > '9') return false;
#if ARDUINO_USB_MODE == 0
    uint8_t raw = s_numpadKeys[digit - '0'];
    usb_hid_kbd().pressRaw(raw);
    delay(2);
    usb_hid_kbd().releaseRaw(raw);
#endif
    return true;
}

void AppBadUSB::_altChar(const char* code)
{
    _kbdPress(KEY_LEFT_ALT);
    while (*code && !_isEnd(*code)) {
        _numpadPress(*code);
        code++;
    }
    _kbdRelease(KEY_LEFT_ALT);
}

bool AppBadUSB::_altString(const char* param)
{
    while (*param) {
        if (*param < ' ' || *param > '~') { param++; continue; }
        char tmp[4];
        snprintf(tmp, sizeof(tmp), "%u", (uint8_t)*param);
        _altChar(tmp);
        param++;
    }
    return true;
}

} // namespace MOONCAKE::APPS
