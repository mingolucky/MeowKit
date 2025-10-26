#pragma once

// Include Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "bsp/devices.h"

class luaEngine {
public:
    luaEngine(std::shared_ptr<DEVICES> device);

    ~luaEngine();

    void runScript(const std::string &scriptContent);

    bool runScriptFromSD(const std::string &id);

    void close();

private:
    lua_State* m_L;
    std::shared_ptr<DEVICES> m_device;
};