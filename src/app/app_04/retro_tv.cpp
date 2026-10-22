/**
 * @file app_04.cpp
 * @brief App04 — Retro TV WiFi screen-cast receiver.
 *
 * Screen layout (320×240):
 *   HDR  [  RETRO-TV  ]                 (()))  ← WiFi icon
 *   ─────────────────────────────────────────
 *        /\   /\    ← antenna tips
 *       ┌──────────────┐
 *       │ · · · · ·    │  ● ● ●  ← knobs
 *       │ · · · · ·    │
 *       └──────────────┘
 *           ║    ║       ← legs
 *
 *        Connect to WiFi...
 *        Saved: MyNetwork
 *   ─────────────────────────────────────────
 *   FTR [+] Navi   [ A ] Connect   [B] Exit
 *
 * WiFi sync: credentials come from system NVS (PKEY_WIFI_SSID / PKEY_WIFI_PASS),
 * the same keys used by Settings > WLAN Setup.
 * ui_wifi_bridge_connect_saved() reads these keys and calls WiFi.begin().
 */
#include "retro_tv.h"
#include "../app_common/mk_tui.h"
#include "../../system/persist.h"
#include <cmath>
#include <WiFi.h>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

namespace MOONCAKE::APPS {

// ─── palette (RGB888) ────────────────────────────────────────────────────────
static constexpr uint32_t C_BG    = 0x0C1400;  // very dark green
static constexpr uint32_t C_FG    = 0xBBE700;  // lime accent (system color)
static constexpr uint32_t C_DIM   = 0x384600;  // dim accent
static constexpr uint32_t C_SCRN  = 0x040800;  // TV screen bg (near-black)
static constexpr uint32_t C_ERR   = 0xFF3333;  // error red

// ─── layout (320×240) ────────────────────────────────────────────────────────
static constexpr int HDR_H  = 24;
static constexpr int FTR_Y  = 220;
static constexpr int TV_CX  = 152;    // TV body horizontal center
static constexpr int TV_TY  = 56;     // TV body top Y (antenna tips clear header at y=23)
static constexpr int TV_BW  = 104;    // TV body width
static constexpr int TV_BH  = 70;     // TV body height

// ─── static draw helpers ─────────────────────────────────────────────────────

static void _drawTV(LGFX_Class& lcd)
{
    int bx = TV_CX - TV_BW / 2;   // 152 - 52 = 100
    int by = TV_TY;                // 50
    int bw = TV_BW;                // 104
    int bh = TV_BH;                // 70

    // Antennas — from upper 1/3 of top edge, going outward-upward
    lcd.drawLine(bx + 28, by, bx + 6,   by - 24, C_FG);  // left
    lcd.drawLine(bx + 76, by, bx + 98,  by - 24, C_FG);  // right
    lcd.fillCircle(bx + 6,  by - 24, 3, C_FG);
    lcd.fillCircle(bx + 98, by - 24, 3, C_FG);

    // Outer body — 2px thick border
    lcd.drawRect(bx,   by,   bw,   bh,   C_FG);
    lcd.drawRect(bx+1, by+1, bw-2, bh-2, C_FG);

    // Screen area (inset, right bezel ~18px for knobs)
    int sx = bx + 6,  sy = by + 6;
    int sw = bw - 24, sh = bh - 12;   // sw=80, sh=58
    lcd.fillRect(sx, sy, sw, sh, C_SCRN);
    lcd.drawRect(sx, sy, sw, sh, C_DIM);

    // Static noise dots (5×3 checkerboard pattern inside screen)
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 5; c++) {
            if ((r + c) & 1) {
                lcd.fillRect(sx + 4 + c * 14, sy + 6 + r * 16, 6, 6, C_DIM);
            }
        }
    }

    // Knobs — 3 circles on right bezel
    int kx = bx + bw - 8;
    lcd.fillCircle(kx, by + bh / 4,       4, C_DIM);
    lcd.fillCircle(kx, by + bh / 2,       4, C_DIM);
    lcd.fillCircle(kx, by + 3 * bh / 4,   4, C_DIM);

    // Legs
    int legW = 12, legH = 10;
    lcd.fillRect(bx + 18, by + bh,         legW, legH, C_FG);
    lcd.fillRect(bx + 74, by + bh,         legW, legH, C_FG);
    lcd.drawFastHLine(bx + 14, by + bh + legH, bw - 28, C_FG);
}

static void _drawHeader(LGFX_Class& lcd)
{
    lcd.fillRect(0, 0, 320, HDR_H, C_BG);
    lcd.drawFastHLine(0, HDR_H - 1, 320, C_FG);
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(C_FG, C_BG);
    lcd.setCursor(8, 4);
    lcd.printf("[ RETRO-TV ]");
}

static void _drawFooter(LGFX_Class& lcd, App04::State st)
{
    lcd.fillRect(0, FTR_Y, 320, 240 - FTR_Y, C_BG);
    lcd.drawFastHLine(0, FTR_Y, 320, C_FG);
    lcd.setFont(&fonts::efontCN_16);

    switch (st) {
        case App04::State::IDLE:
            lcd.setTextColor(C_DIM, C_BG);
            lcd.setCursor(8, FTR_Y + 3);
            lcd.printf("[+] Navi");
            lcd.setTextColor(C_FG, C_BG);
            lcd.setCursor(112, FTR_Y + 3);
            lcd.printf("[ A ] Connect");
            lcd.setTextColor(C_DIM, C_BG);
            lcd.setCursor(260, FTR_Y + 3);
            lcd.printf("[B] Exit");
            break;

        case App04::State::CONNECTING:
            lcd.setTextColor(C_DIM, C_BG);
            lcd.setCursor(248, FTR_Y + 3);
            lcd.printf("[B] Cancel");
            break;

        case App04::State::READY:
            lcd.setTextColor(C_FG, C_BG);
            lcd.setCursor(116, FTR_Y + 3);
            lcd.printf("[ A ] Start");
            lcd.setTextColor(C_DIM, C_BG);
            lcd.setCursor(260, FTR_Y + 3);
            lcd.printf("[B] Exit");
            break;

        case App04::State::RUNNING:
            lcd.setTextColor(C_FG, C_BG);
            lcd.setCursor(8, FTR_Y + 3);
            lcd.printf("Streaming...");
            lcd.setTextColor(C_DIM, C_BG);
            lcd.setCursor(252, FTR_Y + 3);
            lcd.printf("[B] Stop");
            break;
    }
}

// ─── _drawAll ────────────────────────────────────────────────────────────────

void App04::_drawAll()
{
    auto& lcd = _device->Lcd;

    lcd.fillScreen(C_BG);
    _drawHeader(lcd);
    _drawTV(lcd);

    // Status text block — below TV legs
    // TV bottom: TV_TY + TV_BH = 50+70=120. Legs add 10. Platform at y=131.
    int ty = TV_TY + TV_BH + 10 + 16;  // 50+70+10+16 = 146

    lcd.setFont(&fonts::efontCN_16);

    switch (_state) {
        case State::IDLE: {
            const char* msg = "Connect to WiFi...";
            int tw = (int)lcd.textWidth(msg);
            lcd.setTextColor(C_FG, C_BG);
            lcd.setCursor((320 - tw) / 2, ty);
            lcd.printf("%s", msg);

            lcd.setTextColor(C_DIM, C_BG);
            lcd.setCursor(20, ty + 22);
            if (_savedSsid[0])
                lcd.printf("Saved: %.26s", _savedSsid);
            else
                lcd.printf("No saved WiFi - go to Settings > WiFi");
            break;
        }

        case State::CONNECTING: {
            const char* msg = "Connecting...";
            int tw = (int)lcd.textWidth(msg);
            lcd.setTextColor(C_FG, C_BG);
            lcd.setCursor((320 - tw) / 2, ty);
            lcd.printf("%s", msg);

            lcd.setTextColor(C_DIM, C_BG);
            lcd.setCursor(20, ty + 22);
            lcd.printf("SSID: %.26s", _savedSsid);
            break;
        }

        case State::READY: {
            char line1[48];
            snprintf(line1, sizeof(line1), "IP Address: %s", _ip);
            int tw = (int)lcd.textWidth(line1);
            lcd.setTextColor(C_FG, C_BG);
            lcd.setCursor((320 - tw) / 2, ty);
            lcd.printf("%s", line1);

            lcd.setTextColor(C_DIM, C_BG);
            lcd.setCursor(20, ty + 22);
            lcd.printf("Press [A] to start stream");
            break;
        }

        case State::RUNNING:
            // TCP receiver draws full-screen — no overlay needed
            break;
    }

    _drawFooter(lcd, _state);
}

// ─── receiver lifecycle ──────────────────────────────────────────────────────

void App04::_startReceiver()
{
    if (_recv) return;
    try {
        _recv = new TCPReceiver();
        if (_recv) {
            if (!_recv->setup(&_device->Lcd)) {
                delete _recv;
                _recv = nullptr;
                return;
            }
            Serial.println("[App04] TCP receiver started");
        }
    } catch (...) {
        Serial.println("[App04] TCP receiver start failed");
        delete _recv;
        _recv = nullptr;
    }
}

void App04::_stopReceiver()
{
    if (!_recv) return;
    delete _recv;
    _recv = nullptr;
    Serial.println("[App04] TCP receiver stopped");
}

// ─── lifecycle ───────────────────────────────────────────────────────────────

App04::App04(DEVICES* device)
    : _device(device), _recv(nullptr), _isClosing(false), _state(State::IDLE)
{
    setAppInfo().name = "Retro TV";
}

App04::~App04()
{
    if (!_isClosing) onClose();
}

void App04::onOpen()
{
    _isClosing = false;   // reset for re-open after a previous close
    Serial.println("\n[App04] === Retro TV ===");

    // Load saved WiFi SSID from NVS (shared with Settings > WLAN Setup)
    persist_get_str(PKEY_WIFI_SSID, _savedSsid, sizeof(_savedSsid), "");
    memset(_ip, 0, sizeof(_ip));

    if (ui_wifi_bridge_is_connected()) {
        ui_wifi_bridge_get_ip(_ip, sizeof(_ip));
        _state = State::READY;
        Serial.printf("[App04] Already connected — IP=%s\n", _ip);
    } else {
        _state = State::IDLE;
    }

    _drawAll();
}

void App04::onRunning()
{
    _device->button.update();
    _device->button.tick();

    const bool btnA = _device->button.A.pressed();
    const bool btnB = _device->button.B.pressed();

    switch (_state) {

        // ── IDLE: waiting for user to press A ──────────────────────────
        case State::IDLE:
            // Auto-upgrade: if WiFi connected while in IDLE (e.g. after a
            // cancelled CONNECTING), silently advance to READY.
            if (ui_wifi_bridge_is_connected()) {
                ui_wifi_bridge_get_ip(_ip, sizeof(_ip));
                _state = State::READY;
                _drawAll();
                break;
            }
            if (btnA) {
                if (_savedSsid[0] == '\0') {
                    auto& lcd = _device->Lcd;
                    lcd.setFont(&fonts::efontCN_16);
                    int ty = TV_TY + TV_BH + 10 + 16;
                    lcd.fillRect(0, ty + 22, 320, 18, C_BG);
                    lcd.setTextColor(C_ERR, C_BG);
                    lcd.setCursor(20, ty + 22);
                    lcd.printf("No saved WiFi - go to Settings > WiFi");
                } else {
                    Serial.printf("[App04] Connecting to \"%s\"...\n", _savedSsid);
                    ui_wifi_bridge_connect_saved(_savedSsid);
                    _state        = State::CONNECTING;
                    _connectStart = millis();
                    _drawAll();
                }
            }
            if (btnB) close();
            break;

        // ── CONNECTING: poll bridge status, animate dots ───────────────
        case State::CONNECTING: {
            wifi_bridge_status_t status  = ui_wifi_bridge_get_status();
            uint32_t             elapsed = millis() - _connectStart;

            if (status == WIFI_BRIDGE_CONNECTED) {
                ui_wifi_bridge_get_ip(_ip, sizeof(_ip));
                Serial.printf("[App04] Connected — IP=%s\n", _ip);
                _state = State::READY;
                _drawAll();
            } else if (status == WIFI_BRIDGE_FAILED || elapsed > 15000) {
                Serial.println("[App04] Connection failed / timeout");
                _state = State::IDLE;
                _drawAll();
            } else {
                // Animate "Connecting..." dots every 500 ms
                static uint32_t s_lastAnim = 0;
                static int      s_dots     = 0;
                if (millis() - s_lastAnim > 500) {
                    s_lastAnim = millis();
                    s_dots = (s_dots + 1) % 4;
                    auto& lcd = _device->Lcd;
                    lcd.setFont(&fonts::efontCN_16);
                    int ty = TV_TY + TV_BH + 10 + 16;
                    lcd.fillRect(0, ty, 320, 18, C_BG);
                    char animBuf[20];
                    snprintf(animBuf, sizeof(animBuf), "Connecting%.*s", s_dots, "...");
                    int tw = (int)lcd.textWidth(animBuf);
                    lcd.setTextColor(C_FG, C_BG);
                    lcd.setCursor((320 - tw) / 2, ty);
                    lcd.printf("%s", animBuf);
                }
            }

            if (btnB) {
                // Cancel: go back to IDLE (WiFi keeps trying in background)
                _state = State::IDLE;
                _drawAll();
            }
            break;
        }

        // ── READY: WiFi connected, waiting for [A] to start stream ─────
        case State::READY:
            if (btnA) {
                _startReceiver();
                if (!_recv) break;
                _state = State::RUNNING;
                // Receiver draws full-screen — just update footer
                _drawFooter(_device->Lcd, _state);
            }
            if (btnB) close();
            break;

        // ── RUNNING: streaming ─────────────────────────────────────────
        case State::RUNNING:
            if (_recv) {
                try { _recv->loop(); } catch (...) {}
            }
            // Return to ready screen on B press, or if WiFi drops
            if (btnB || !ui_wifi_bridge_is_connected()) {
                _stopReceiver();
                if (ui_wifi_bridge_is_connected()) {
                    _state = State::READY;
                } else {
                    _state = State::IDLE;
                }
                _drawAll();
            }
            break;
    }
}

void App04::onClose()
{
    if (_isClosing) return;
    _isClosing = true;
    Serial.println("[App04] closed");
    _stopReceiver();
    /* Launcher is responsible for its own screen redraw — no fillScreen here. */
}

}  // namespace MOONCAKE::APPS
