#pragma once

#include <lua.hpp>
#include <string>

// Forward declaration of device class
class DEVICES;

/**
 * @class LuaDevice
 * @brief Lua device interface class, providing Lua bindings for hardware devices
 * 
 * This class encapsulates device operations, making them callable from Lua scripts
 */
class LuaDevice
{
public:
    /**
     * @brief Constructor
     * @param device Pointer to the device object
     */
    explicit LuaDevice(DEVICES *device);

    /**
     * @brief Clear the screen
     */
    void clearScreen();

    /**
     * @brief Get screen dimensions
     * @param[out] width Screen width
     * @param[out] height Screen height
     */
    void getScreenSize(int &width, int &height);

    /**
     * @brief Draw a pixel
     * @param x X-coordinate
     * @param y Y-coordinate
     * @param color Color value
     */
    void drawPixel(int x, int y, int color);

    /**
     * @brief Print text to the screen
     * @param text Text to print
     */
    void screnenPrint(const std::string &text);


    /**
     * @brief Register the device class to Lua state machine
     * @param L Lua state machine pointer
     * @param device Device object pointer
     */
    static void registerToLua(lua_State *L, DEVICES *device);

private:
    DEVICES *m_device; ///< Pointer to the underlying device object
};

// Lua binding function declarations (in a separate namespace)
namespace lua_bind
{
    int clearScreen(lua_State *L);
    int getScreenSize(lua_State *L);
    int drawPixel(lua_State *L);
    int screenPrint(lua_State *L);
}
