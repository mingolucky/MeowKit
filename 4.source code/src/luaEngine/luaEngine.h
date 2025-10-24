// Include Lua headers
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "bsp/devices.h"

class luaEngine { 
public:
    luaEngine(DEVICES *device);
    ~luaEngine();
    void runScript(const char* description);
    bool runScriptFromSD(const std::string& id);
    void close();

private:
    lua_State* m_L;
    DEVICES *m_device;
};