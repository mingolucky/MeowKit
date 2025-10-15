/**
 * @file badusb.cpp
 * @author Mingo
 * @brief BadUSB app implementation for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */

#include "bad_usb.h"
using std::vector;

//Button icon resources (same as the MUSIC app)
#include "ui/src/ui.h"
extern const lv_img_dsc_t ui_img_ui_normal_btn_a_png;
extern const lv_img_dsc_t ui_img_ui_normal_btn_b_png;

namespace MOONCAKE::APPS
{
    BadUSB::BadUSB(DEVICES* device)
        : _device(device)
    {
        setAppInfo().name = "BadUSB";
    }

    void BadUSB::onOpen()
    {
        // Show Basic UI
        _device->Lcd.fillScreen(TFT_BLACK);
        _device->Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        _device->Lcd.setFont(&fonts::efontCN_16);

        // Enable USB HID keyboard
        USB.begin();
        Keyboard.begin();

        // Initialize SD
        _sdReady = _device->sd.isInited() ? true : _device->sd.init();

        // Scanning Script
        scanScripts();
        resetExec();
        redrawAll(_sdReady ? "Ready" : "No SD");
    }

    void BadUSB::onRunning()
    {
        // Key events
        _device->button.A.tick();
        _device->button.B.tick();

        // A single click: Start/Continue; A double click: Restart the current script
        if (_device->button.A.isSingleClick()) {
            if (!_running) {
                if (loadCurrentScript()) {
                    _running = true;
                    _paused = false;
                    drawStatus("Running");
                } else {
                    drawStatus("Load fail");
                }
            } else {
                _paused = !_paused;
                drawStatus(_paused ? "Paused" : "Running");
            }
        }
        if (_device->button.A.isDoubleClick()) {
            if (loadCurrentScript()) {
                _running = true;
                _paused = false;
                drawStatus("Restarted");
            }
        }

        // B single click: next script; B double click: previous script (switch when not running, pause before switching when running)
        if (_device->button.B.isSingleClick()) {
            if (_running) { _paused = true; _running = false; }
            if (!_scripts.empty()) {
                _sel = (_sel + 1) % _scripts.size();
                redrawAll("Selected");
            }
        }
        if (_device->button.B.isDoubleClick()) {
            if (_running) { _paused = true; _running = false; }
            if (!_scripts.empty()) {
                int n = (int)_scripts.size();
                _sel = (_sel - 1 + n) % n;
                redrawAll("Selected");
            }
        }

        // Non-blocking script execution
        if (_running && !_paused) {
            if (millis() >= _nextTime) {
                stepExecution();
            }
        }
    }

             

    void BadUSB::onClose()
    {
        Keyboard.end();         // Disable HID keyboard functionality
        _device->Lcd.fillScreen(TFT_BLACK);
    }

    // ===== Internal Implementation ===== //

    void BadUSB::drawHeader() {
        _device->Lcd.fillScreen(TFT_BLACK);
        _device->Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        _device->Lcd.setCursor(10, 10);
        _device->Lcd.print("BadUSB");
        _device->Lcd.setCursor(10, 30);
        _device->Lcd.print(_sdReady ? "SD: OK" : "SD: FAIL");
    }

    void BadUSB::drawSelection() {
        int y = 50;
        _device->Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        if (_scripts.empty()) {
            _device->Lcd.setCursor(10, y);
            _device->Lcd.print("/badusb/*.txt not found");
            return;
        }
        // Display the current selection and the two adjacent
        for (int i = -1; i <= 1; ++i) {
            int idx = (int)_scripts.size() == 0 ? 0 : (int)((_sel + _scripts.size() + i) % _scripts.size());
            String name = _scripts[idx];
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);
            _device->Lcd.setCursor(10, y);
            if (i == 0) {
                _device->Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                _device->Lcd.print("> ");
                _device->Lcd.print(name);
                _device->Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            } else {
                _device->Lcd.print("  ");
                _device->Lcd.print(name);
            }
            y += 20;
        }
    }

    void BadUSB::drawStatus(const char* status) {
    // The status bar display has been disabled to avoid blocking the A/B button icons and text
    (void)status;
    return;
    }

    void BadUSB::redrawAll(const char* status) {
        drawHeader();
        drawSelection();
        // Draw A/B button icons and function labels
        {
            // Display button texture with alpha using MUSIC drawing mode
            _device->Lcd.setSwapBytes(true); // Make RGB565 endian consistent with source material

            int cx = _device->Lcd.width() / 2;
            int cy = _device->Lcd.height() / 2;

            // ---- Draw the A button ----
            {
                int aW = ui_img_ui_normal_btn_a_png.header.w;
                int aH = ui_img_ui_normal_btn_a_png.header.h;
                // Position: Near the bottom, centered slightly to the left (parameters similar to MUSIC)
                int a_center_x = (cx - 45) - 55;
                int a_center_y = cy + 100;
                int a_x = a_center_x - aW / 2;
                int a_y = a_center_y - aH / 2;

                static uint16_t lineBuf[64];
                const uint8_t* src = (const uint8_t*)ui_img_ui_normal_btn_a_png.data;
                int w = aW, h = aH;
                for (int y = 0; y < h; ++y) {
                    const uint8_t* row = src + (y * w * 3); // 2B RGB565 + 1B Alpha
                    int runStart = -1, runLen = 0;
                    for (int x = 0; x < w; ++x) {
                        uint16_t c565 = (uint16_t)(row[x*3 + 0] | (row[x*3 + 1] << 8));
                        uint8_t a = row[x*3 + 2];
                        if (a == 0) {
                            if (runLen > 0) {
                                _device->Lcd.pushImage(a_x + runStart, a_y + y, runLen, 1, lineBuf);
                                runStart = -1; runLen = 0;
                            }
                            continue;
                        }
                        if (runLen == 0) runStart = x;
                        lineBuf[runLen++] = c565;
                        if (runLen == (int)(sizeof(lineBuf)/sizeof(lineBuf[0]))) {
                            _device->Lcd.pushImage(a_x + runStart, a_y + y, runLen, 1, lineBuf);
                            runStart = -1; runLen = 0;
                        }
                    }
                    if (runLen > 0) {
                        _device->Lcd.pushImage(a_x + runStart, a_y + y, runLen, 1, lineBuf);
                    }

                // Draw English function label on the right side of the button (single click/double click)
                _device->Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                _device->Lcd.setFont(&fonts::efontCN_16);
                _device->Lcd.setCursor(a_x + aW + 6, a_center_y - 10);
                _device->Lcd.print("Start/Continue");
                }
            }

            // ---- Draw the B button ----
            {
                int bW = ui_img_ui_normal_btn_b_png.header.w;
                int bH = ui_img_ui_normal_btn_b_png.header.h;
                int b_center_x = cx + 70 - 10;
                int b_center_y = cy + 100;
                int b_x = b_center_x - bW / 2;
                int b_y = b_center_y - bH / 2;

                static uint16_t lineBuf[64];
                const uint8_t* src = (const uint8_t*)ui_img_ui_normal_btn_b_png.data;
                int w = bW, h = bH;
                for (int y = 0; y < h; ++y) {
                    const uint8_t* row = src + (y * w * 3);
                    int runStart = -1, runLen = 0;
                    for (int x = 0; x < w; ++x) {
                        uint16_t c565 = (uint16_t)(row[x*3 + 0] | (row[x*3 + 1] << 8));
                        uint8_t a = row[x*3 + 2];
                        if (a == 0) {
                            if (runLen > 0) {
                                _device->Lcd.pushImage(b_x + runStart, b_y + y, runLen, 1, lineBuf);
                                runStart = -1; runLen = 0;
                            }
                            continue;
                        }
                        if (runLen == 0) runStart = x;
                        lineBuf[runLen++] = c565;
                        if (runLen == (int)(sizeof(lineBuf)/sizeof(lineBuf[0]))) {
                            _device->Lcd.pushImage(b_x + runStart, b_y + y, runLen, 1, lineBuf);
                            runStart = -1; runLen = 0;
                        }
                    }
                    if (runLen > 0) {
                        _device->Lcd.pushImage(b_x + runStart, b_y + y, runLen, 1, lineBuf);
                    }

                // Draw the English function label on the right side of the button (B: script switching)
                _device->Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                _device->Lcd.setFont(&fonts::efontCN_16);
                _device->Lcd.setCursor(b_x + bW + 6, b_center_y - 8);
                _device->Lcd.print("Prev/Next");
                }
            }

            _device->Lcd.setSwapBytes(false); // Restore Defaults
        }
        drawStatus(status);
    }

    void BadUSB::scanScripts() {
        _scripts.clear();
        if (!_sdReady) return;
        File dir = SD_MMC.open("/badusb");
        if (!dir || !dir.isDirectory()) return;
        File f = dir.openNextFile();
        while (f) {
            if (!f.isDirectory()) {
                String n = f.name();
                if (n.endsWith(".txt") || n.endsWith(".TXT")) {
                    if (!n.startsWith("/badusb")) n = String("/badusb/") + n; // Some platforms return relative names
                    _scripts.push_back(n);
                }
            }
            f.close();
            f = dir.openNextFile();
        }
        dir.close();
        if (_sel >= (int)_scripts.size()) _sel = 0;
    }

    bool BadUSB::loadCurrentScript() {
        _lines.clear();
        _ip = 0;
        if (_scripts.empty()) return false;
        String path = _scripts[_sel];
        File f = SD_MMC.open(path.c_str());
        if (!f) return false;
        while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            // Skip blank lines and comments (# or //)
            if (line.length() == 0) continue;
            if (line.startsWith("#")) continue;
            if (line.startsWith("//")) continue;
            _lines.push_back(line);
        }
        f.close();
        return !_lines.empty();
    }

    void BadUSB::resetExec() {
        _running = false;
        _paused = false;
        _ip = 0;
        _nextTime = millis();
    }

    void BadUSB::stepExecution() {
        if (_ip >= _lines.size()) {
            _running = false;
            drawStatus("Done");
            return;
        }
        String line = _lines[_ip++];
        execCommand(line);
    }

    bool BadUSB::matchToken(const String& s, const char* token) {
        size_t n = strlen(token);
        if (s.length() < n) return false;
        for (size_t i = 0; i < n; ++i) {
            char a = tolower(s[i]);
            char b = tolower(token[i]);
            if (a != b) return false;
        }
        // There can be a space or end after the token
        if (s.length() == n) return true;
        return s[n] == ' ' || s[n] == '\t';
    }

    uint8_t BadUSB::keyFromName(const String& name, bool& isPrintable, uint8_t& printableChar) {
        String n = name; n.trim();
        // Single character printable
        if (n.length() == 1) {
            isPrintable = true;
            printableChar = (uint8_t)n[0];
            return 0;
        }
        isPrintable = false;
        printableChar = 0;
        // Function key mapping (extensible)
        if (n.equalsIgnoreCase("ENTER") || n.equalsIgnoreCase("RETURN")) return KEY_RETURN;
        if (n.equalsIgnoreCase("ESC") || n.equalsIgnoreCase("ESCAPE")) return KEY_ESC;
        if (n.equalsIgnoreCase("TAB")) return KEY_TAB;
        if (n.equalsIgnoreCase("SPACE")) { isPrintable = true; printableChar = ' '; return 0; }
        if (n.equalsIgnoreCase("UP")) return KEY_UP_ARROW;
        if (n.equalsIgnoreCase("DOWN")) return KEY_DOWN_ARROW;
        if (n.equalsIgnoreCase("LEFT")) return KEY_LEFT_ARROW;
        if (n.equalsIgnoreCase("RIGHT")) return KEY_RIGHT_ARROW;
        if (n.equalsIgnoreCase("F1")) return KEY_F1;
        if (n.equalsIgnoreCase("F2")) return KEY_F2;
        if (n.equalsIgnoreCase("F3")) return KEY_F3;
        if (n.equalsIgnoreCase("F4")) return KEY_F4;
        if (n.equalsIgnoreCase("F5")) return KEY_F5;
        if (n.equalsIgnoreCase("F6")) return KEY_F6;
        if (n.equalsIgnoreCase("F7")) return KEY_F7;
        if (n.equalsIgnoreCase("F8")) return KEY_F8;
        if (n.equalsIgnoreCase("F9")) return KEY_F9;
        if (n.equalsIgnoreCase("F10")) return KEY_F10;
        if (n.equalsIgnoreCase("F11")) return KEY_F11;
        if (n.equalsIgnoreCase("F12")) return KEY_F12;
        // Numbers and letters
        if (n.length() == 1 && isalnum((int)n[0])) { isPrintable = true; printableChar = (uint8_t)n[0]; return 0; }
        return 0; // Unidentified
    }

    void BadUSB::execCommand(const String& raw) {
        String line = raw; // Instances
        // Supported commands (simplified DuckyScript):
        // REM <comment>
        // DELAY <ms>
        // STRING <text>
        // ENTER / RETURN
        // GUI|WINDOWS <key?>
        // ALT <key>
        // CTRL <key>
        // SHIFT <key>
        // TAB
        // F1..F12, UP/DOWN/LEFT/RIGHT
        // TEXT <printable tokens separated by spaces>

        if (matchToken(line, "REM")) {
            // Comment, skip
            _nextTime = millis();
            return;
        }
        if (matchToken(line, "DELAY")) {
            int sp = line.indexOf(' ');
            int ms = (sp >= 0) ? line.substring(sp + 1).toInt() : 0;
            if (ms < 0) ms = 0;
            _nextTime = millis() + (uint32_t)ms;
            return;
        }
        if (matchToken(line, "STRING")) {
            int sp = line.indexOf(' ');
            String text = (sp >= 0) ? line.substring(sp + 1) : "";
            if (text.length() > 0) Keyboard.print(text);
            _nextTime = millis();
            return;
        }
        if (matchToken(line, "ENTER") || matchToken(line, "RETURN")) {
            Keyboard.write(KEY_RETURN);
            _nextTime = millis();
            return;
        }
        if (matchToken(line, "TAB")) {
            Keyboard.write(KEY_TAB);
            _nextTime = millis();
            return;
        }
        if (matchToken(line, "GUI") || matchToken(line, "WINDOWS")) {
            int sp = line.indexOf(' ');
            if (sp < 0) {
                Keyboard.write(KEY_LEFT_GUI);
            } else {
                String k = line.substring(sp + 1); k.trim();
                bool printable=false; uint8_t ch=0; uint8_t vk = keyFromName(k, printable, ch);
                Keyboard.press(KEY_LEFT_GUI);
                delay(10);
                if (printable) Keyboard.press(ch); else if (vk) Keyboard.press(vk);
                delay(20);
                Keyboard.releaseAll();
            }
            _nextTime = millis();
            return;
        }
        if (matchToken(line, "ALT") || matchToken(line, "CTRL") || matchToken(line, "SHIFT")) {
            bool isAlt = matchToken(line, "ALT");
            bool isCtrl = matchToken(line, "CTRL");
            bool isShift = matchToken(line, "SHIFT");
            int sp = line.indexOf(' ');
            if (sp >= 0) {
                String k = line.substring(sp + 1); k.trim();
                bool printable=false; uint8_t ch=0; uint8_t vk = keyFromName(k, printable, ch);
                if (isAlt) Keyboard.press(KEY_LEFT_ALT);
                if (isCtrl) Keyboard.press(KEY_LEFT_CTRL);
                if (isShift) Keyboard.press(KEY_LEFT_SHIFT);
                delay(10);
                if (printable) Keyboard.press(ch); else if (vk) Keyboard.press(vk);
                delay(20);
                Keyboard.releaseAll();
            }
            _nextTime = millis();
            return;
        }

        // Direct function keys or text:
        // Example: F11 / UP / a b c
        {
            // Split by space and send one by one
            int start = 0;
            while (start < (int)line.length()) {
                while (start < (int)line.length() && line[start] == ' ') start++;
                if (start >= (int)line.length()) break;
                int end = start;
                while (end < (int)line.length() && line[end] != ' ') end++;
                String tok = line.substring(start, end);
                bool printable=false; uint8_t ch=0; uint8_t vk = keyFromName(tok, printable, ch);
                if (printable) Keyboard.write(ch);
                else if (vk) Keyboard.write(vk);
                start = end + 1;
            }
            _nextTime = millis();
            return;
        }
    }
}
