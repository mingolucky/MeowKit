#include "luaDeviceLib.h"
#include "bsp/devices.h" // Include device definitions
#include <stdexcept>     // Exception handling

// Constructor implementation
LuaDevice::LuaDevice(DEVICES *device) : m_device(device)
{
    if (!device)
    {
        throw std::invalid_argument("Device pointer cannot be null");
    }
}

// Clear screen implementation
void LuaDevice::clearScreen()
{
    if (m_device)
    {
        m_device->Lcd.fillScreen(TFT_BLACK);
    }
}

// Get screen dimensions implementation
void LuaDevice::getScreenSize(int &width, int &height)
{
    if (m_device)
    {
        width = m_device->Lcd.width();
        height = m_device->Lcd.height();
    }
}

// Draw pixel implementation
void LuaDevice::drawPixel(int x, int y, int color)
{
    if (m_device)
    {
        m_device->Lcd.drawPixel(x, y, color);
    }
}








//*****************************lua binding functions************************************ */
// Lua binding functions implementation
int lua_bind::clearScreen(lua_State *L)
{
    // Parameter validation
    if (lua_gettop(L) != 1)
    {
        return luaL_error(L, "clearScreen expects 1 argument (self)");
    }

    // Get device instance
    LuaDevice **ud = (LuaDevice **)luaL_checkudata(L, 1, "LuaDevice");
    if (!ud || !*ud)
    {
        return luaL_error(L, "Invalid LuaDevice instance");
    }

    // Execute operation
    try
    {
        (*ud)->clearScreen();
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "clearScreen failed: %s", e.what());
    }
    return 0;
}

int lua_bind::getScreenSize(lua_State *L)
{
    // Parameter validation
    if (lua_gettop(L) != 1)
    {
        return luaL_error(L, "getScreenSize expects 1 argument (self)");
    }

    // Get device instance
    LuaDevice **ud = (LuaDevice **)luaL_checkudata(L, 1, "LuaDevice");
    if (!ud || !*ud)
    {
        return luaL_error(L, "Invalid LuaDevice instance");
    }

    // Execute operation
    try
    {
        int width, height;
        (*ud)->getScreenSize(width, height);
        lua_pushinteger(L, width);
        lua_pushinteger(L, height);
        return 2; // Return two values
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "getScreenSize failed: %s", e.what());
    }
}

int lua_bind::drawPixel(lua_State *L)
{
    // Parameter validation
    if (lua_gettop(L) != 4)
    {
        return luaL_error(L, "drawPixel expects 4 arguments (self, x, y, color)");
    }

    // Get device instance
    LuaDevice **ud = (LuaDevice **)luaL_checkudata(L, 1, "LuaDevice");
    if (!ud || !*ud)
    {
        return luaL_error(L, "Invalid LuaDevice instance");
    }

    // Get parameters
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int color = luaL_checkinteger(L, 4);

    // Execute operation
    try
    {
        (*ud)->drawPixel(x, y, color);
    }
    catch (const std::exception &e)
    {
        return luaL_error(L, "drawPixel failed: %s", e.what());
    }
    return 0;
}

// Registration function implementation
void LuaDevice::registerToLua(lua_State *L, DEVICES *device)
{
    // Create metatable
    luaL_newmetatable(L, "LuaDevice");
    lua_pushvalue(L, -1);           // Duplicate metatable
    lua_setfield(L, -2, "__index"); // Set __index = metatable

    // Register methods
    luaL_Reg methods[] = {
        {"clearScreen", lua_bind::clearScreen},
        {"getScreenSize", lua_bind::getScreenSize},
        {"drawPixel", lua_bind::drawPixel},
        {nullptr, nullptr}};
    luaL_setfuncs(L, methods, 0);

    // Create device instance
    LuaDevice **ud = (LuaDevice **)lua_newuserdata(L, sizeof(LuaDevice *));
    *ud = new LuaDevice(device);

    // Set metatable
    luaL_setmetatable(L, "LuaDevice");

    // Set as global variable
    lua_setglobal(L, "Device");
}
