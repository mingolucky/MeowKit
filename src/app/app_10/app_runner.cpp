/**
 * @file app_runner.cpp
 * @brief App10 — Lua App Runner implementation.
 */
#include "app_runner.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include "../../system/lua_host/lua_host.hpp"
#include "../../system/lua_host/mk_bindings.hpp"

extern "C" {
#include "lauxlib.h"
}

namespace {
constexpr uint32_t COL_BG     = 0x000000;
constexpr uint32_t COL_ACCENT = 0xBBE700; // MeowKit green
constexpr uint32_t COL_TEXT   = 0xFFFFFF;
constexpr uint32_t COL_DIM    = 0x888888;
constexpr uint32_t COL_SEL    = 0x00FF66;
constexpr uint32_t COL_ERR    = 0xFF5555;

const char* lastError(lua_State* L)
{
    const char* e = lua_tostring(L, -1);
    return e ? e : "(unknown error)";
}
} // namespace

namespace MOONCAKE::APPS
{
    AppRunner::AppRunner(DEVICES* device) : _device(device)
    {
        setAppInfo().name = "Lua Apps";
    }

    void AppRunner::scanApps()
    {
        _apps.clear();
        File root = SD_MMC.open("/apps");
        if (!root || !root.isDirectory()) {
            if (root) root.close();
            return;
        }
        for (File e = root.openNextFile(); e; e = root.openNextFile()) {
            if (e.isDirectory()) {
                const char* full = e.name();
                const char* slash = strrchr(full, '/');
                std::string name = slash ? slash + 1 : full;
                std::string main = "/apps/" + name + "/main.lua";
                if (SD_MMC.exists(main.c_str())) _apps.push_back(name);
            }
            e.close();
        }
        root.close();
    }

    void AppRunner::onOpen()
    {
        _device->Lcd.fillScreen(COL_BG);
        scanApps();
        _sel  = 0;
        _mode = Mode::Menu;

        // Seed edge-detect with current levels so the A press that launched this
        // app (or a held button) doesn't register as a fresh edge.
        _prev[0] = _device->button.A.state()     == Button_Class::PRESSED;
        _prev[1] = _device->button.B.state()     == Button_Class::PRESSED;
        _prev[2] = _device->button.Up.state()    == Button_Class::PRESSED;
        _prev[3] = _device->button.Down.state()  == Button_Class::PRESSED;
        _prev[4] = _device->button.Left.state()  == Button_Class::PRESSED;
        _prev[5] = _device->button.Right.state() == Button_Class::PRESSED;

        drawMenu();
    }

    void AppRunner::drawMenu()
    {
        auto& lcd = _device->Lcd;
        lcd.fillScreen(COL_BG);
        lcd.setTextColor(COL_ACCENT);
        lcd.setTextSize(2);
        lcd.drawString("Lua Apps", 10, 8);

        lcd.setTextSize(1);
        if (_apps.empty()) {
            lcd.setTextColor(COL_TEXT);
            lcd.drawString("No scripts in /apps/ on SD.", 10, 54);
            lcd.setTextColor(COL_DIM);
            lcd.drawString("Add /apps/<name>/main.lua", 10, 74);
            lcd.drawString("to a FAT32 card.", 10, 90);
        } else {
            for (int i = 0; i < (int)_apps.size() && i < 8; ++i) {
                int y = 44 + i * 22;
                bool sel = (i == _sel);
                lcd.setTextColor(sel ? COL_SEL : COL_TEXT);
                lcd.drawString((sel ? "> " : "  ") + String(_apps[i].c_str()), 14, y);
            }
        }

        lcd.setTextColor(COL_DIM);
        lcd.drawString("Up/Down  A:run  hold B:exit", 10, 222);
    }

    void AppRunner::drawError()
    {
        auto& lcd = _device->Lcd;
        lcd.fillScreen(COL_BG);
        lcd.setTextColor(COL_ERR);
        lcd.setTextSize(2);
        lcd.drawString("Script error", 10, 8);

        lcd.setTextColor(COL_TEXT);
        lcd.setTextSize(1);
        // Wrap the message across lines at ~50 chars.
        const std::string& m = _error;
        int line = 0;
        for (size_t i = 0; i < m.size() && line < 11; i += 50, ++line) {
            lcd.drawString(m.substr(i, 50).c_str(), 10, 44 + line * 14);
        }
        lcd.setTextColor(COL_DIM);
        lcd.drawString("A/B: back to list", 10, 222);
    }

    bool AppRunner::callOptional(const char* fn, bool withDt, uint32_t dt, std::string& err)
    {
        lua_getglobal(_lua, fn);
        if (!lua_isfunction(_lua, -1)) { lua_pop(_lua, 1); return true; }
        int nargs = 0;
        if (withDt) { lua_pushinteger(_lua, (lua_Integer)dt); nargs = 1; }
        if (lua_pcall(_lua, nargs, 0, 0) != LUA_OK) {
            err = lastError(_lua);
            lua_pop(_lua, 1);
            return false;
        }
        return true;
    }

    void AppRunner::launch(const std::string& name)
    {
        _current = name;
        _lua = lua_host::newState();
        if (_lua == nullptr) { toError("Lua VM alloc failed (PSRAM?)"); return; }
        mk_bindings::install(_lua, _device);
        mk_bindings::clearExit();

        std::string path = "/apps/" + name + "/main.lua";
        File f = SD_MMC.open(path.c_str(), FILE_READ);
        if (!f || f.isDirectory()) { if (f) f.close(); toError("cannot open " + path); return; }
        std::string src;
        src.resize(f.size());
        if (!src.empty()) f.read((uint8_t*)&src[0], src.size());
        f.close();

        _device->Lcd.fillScreen(COL_BG);
        if (luaL_loadbuffer(_lua, src.data(), src.size(), name.c_str()) != LUA_OK) {
            std::string e = lastError(_lua);
            toError(e);
            return;
        }
        if (lua_pcall(_lua, 0, 0, 0) != LUA_OK) {  // run top-level chunk
            std::string e = lastError(_lua);
            toError(e);
            return;
        }

        std::string err;
        if (!callOptional("on_open", false, 0, err)) { toError(err); return; }

        _mode    = Mode::Running;
        _last_ms = millis();
        Serial.printf("[AppRunner] running /apps/%s/main.lua\n", name.c_str());
    }

    void AppRunner::toMenu()
    {
        if (_lua) {
            std::string err;
            callOptional("on_close", false, 0, err); // best-effort
            lua_close(_lua);
            _lua = nullptr;
        }
        mk_bindings::clearExit();
        _mode = Mode::Menu;
        if (_sel >= (int)_apps.size()) _sel = 0;
        drawMenu();
    }

    void AppRunner::toError(const std::string& msg)
    {
        if (_lua) { lua_close(_lua); _lua = nullptr; }
        _error = msg;
        _mode  = Mode::Error;
        Serial.printf("[AppRunner] error: %s\n", msg.c_str());
        drawError();
    }

    void AppRunner::onRunning()
    {
        // Edge-detect from debounced levels (the launcher already ticked buttons
        // this loop). Reading levels does not consume the launcher's long-press-B.
        bool cur[6] = {
            _device->button.A.state()     == Button_Class::PRESSED,
            _device->button.B.state()     == Button_Class::PRESSED,
            _device->button.Up.state()    == Button_Class::PRESSED,
            _device->button.Down.state()  == Button_Class::PRESSED,
            _device->button.Left.state()  == Button_Class::PRESSED,
            _device->button.Right.state() == Button_Class::PRESSED,
        };
        bool edge[6];
        for (int i = 0; i < 6; ++i) { edge[i] = cur[i] && !_prev[i]; _prev[i] = cur[i]; }
        const bool eA = edge[0], eB = edge[1], eUp = edge[2], eDown = edge[3];

        switch (_mode) {
        case Mode::Menu:
            if (!_apps.empty()) {
                if (eUp)   { _sel = (_sel - 1 + (int)_apps.size()) % (int)_apps.size(); drawMenu(); }
                if (eDown) { _sel = (_sel + 1) % (int)_apps.size();                     drawMenu(); }
                if (eA)    { launch(_apps[_sel]); }
            }
            break;

        case Mode::Running: {
            if (eB) { toMenu(); break; } // short B → back to list

            uint32_t now = millis();
            uint32_t dt  = now - _last_ms;
            _last_ms = now;

            std::string err;
            if (!callOptional("on_running", true, dt, err)) { toError(err); break; }
            if (_mode == Mode::Running && mk_bindings::exitRequested()) { toMenu(); }
            break;
        }

        case Mode::Error:
            if (eA || eB) { toMenu(); }
            break;
        }
    }

    void AppRunner::onClose()
    {
        if (_lua) {
            std::string err;
            callOptional("on_close", false, 0, err);
            lua_close(_lua);
            _lua = nullptr;
        }
        mk_bindings::clearExit();
        if (_device->Lcd.width() > 0) _device->Lcd.fillScreen(COL_BG);
    }
}
