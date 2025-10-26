#include "luaEngine.h"
#include "luaSDK.h"
#include "esp_log.h"
#include "luaScriptManager.h"
#include "luaDeviceLib.h"


static const char *TAG = "luaEngine";

luaEngine::luaEngine(std::shared_ptr<DEVICES> device) : m_device(device),
                                        m_L(nullptr)
{
}

luaEngine::~luaEngine()
{
    close();
}

void luaEngine::close()
{
    if (m_L != nullptr)
    {
        lua_close(m_L);
        m_L = nullptr;
    }
}

void luaEngine::runScript(const std::string& scriptContent)
{
    lua_State *L = luaL_newstate();
    if (L == NULL)
    {
        ESP_LOGE(TAG, "Failed to create Lua state");
        return;
    }

    luaL_openlibs(L);

    // Set Lua module search path
    if (luaL_dostring(L, "package.path = package.path .. ';./?.lua;/script/?.lua;/spiffs/?.lua'"))
    {
        ESP_LOGE(TAG, "Failed to set package.path: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    // Load and execute the Lua script from file
    if (luaL_dofile(L, scriptContent.c_str()) != LUA_OK)
    {
        ESP_LOGE(TAG, "Error executing Lua script: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    lua_close(L);
    ESP_LOGI(TAG, "Finished running Lua script");
}

bool luaEngine::runScriptFromSD(const std::string& id)
{
    std::string scriptContent = "";
    if (!luaScriptManager::getInstance().getScriptById(id, scriptContent))
    {
        ESP_LOGE(TAG, "Failed to create Lua state");
        return false;
    }
    // close previous lua state
    if (m_L != nullptr)
    {
        lua_close(m_L);
        m_L = nullptr;
        vTaskDelay(10); 
    }

    m_L = luaL_newstate();
    if (m_L == nullptr)
    {
        ESP_LOGE(TAG, "Failed to create Lua state");
        return false;
    }

    // open standard libraries
    luaL_openlibs(m_L);

    LuaDevice::registerToLua(m_L, m_device.get());

    luaL_requiref(m_L, "myTable", Lua_ModelTest, 1);

    // Execute Lua code
    if (luaL_dostring(m_L, scriptContent.c_str()) != LUA_OK)
    {
        ESP_LOGE(TAG, "Error executing Lua script: %s", lua_tostring(m_L, -1));
        lua_pop(m_L, 1);
    }

    lua_close(m_L);
    m_L = nullptr;
    ESP_LOGI(TAG, "Finished running Lua script");

    return true;
}