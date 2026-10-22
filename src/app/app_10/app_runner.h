/**
 * @file app_runner.h
 * @brief App10 — Lua App Runner: loads and runs plugin scripts from the SD card.
 *
 * Scans /apps/<name>/main.lua on the SD card and runs the selected script in a
 * PSRAM-backed Lua VM with the mk.* device bindings. Every script entry point
 * is pcall-wrapped: a script error drops to an on-screen message, never a
 * device crash. Long-press B (handled by the launcher) always exits; a short B
 * returns from a running script to the script list.
 *
 * Script lifecycle (all optional globals):
 *   on_open()            -- once, after load
 *   on_running(dt_ms)    --每 tick; keep it short
 *   on_close()           -- on exit / back
 */
#pragma once

#include <mooncake.h>
#include "../../bsp/devices.h"
#include <string>
#include <vector>

extern "C" {
#include "lua.h"
}

using namespace mooncake;

namespace MOONCAKE::APPS
{
    class AppRunner : public AppAbility {
    public:
        AppRunner(DEVICES* device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        enum class Mode { Menu, Running, Error };

        DEVICES*                 _device = nullptr;
        lua_State*               _lua      = nullptr;
        std::vector<std::string> _apps;
        int                      _sel    = 0;
        Mode                     _mode   = Mode::Menu;
        std::string              _error;
        std::string              _current;
        uint32_t                 _last_ms = 0;
        bool                     _prev[6] = { false, false, false, false, false, false };

        void scanApps();
        void drawMenu();
        void drawError();
        void launch(const std::string& name);
        void toMenu();
        void toError(const std::string& msg);
        bool callOptional(const char* fn, bool withDt, uint32_t dt, std::string& err);
    };
}
