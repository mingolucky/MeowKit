#pragma once

#include "bsp/devices.h"
#include <map>
#include <string>
#include <optional>

class luaScriptManager {
public:
    // Delete constructor and assignment operator
    luaScriptManager(const luaScriptManager&) = delete;
    luaScriptManager& operator=(const luaScriptManager&) = delete;
    
    // Get singleton instance
    static luaScriptManager& getInstance();
    
    // Read all folders starting with app_id under the /script folder on the SD card
    // and load the script file named app_id.lua from each folder
    bool loadScriptsFromSD();
    
    // Run the script with the specified ID
    bool getScriptById(const std::string& id, std::string& outScript) const;
    
    // Get a list of loaded scripts
    const std::map<std::string, std::string>& getLoadedScripts() const;

    // Set device pointer
    void setDevice(DEVICES* device);

private:
    luaScriptManager();  // Private constructor
    ~luaScriptManager();

    DEVICES* m_device;
    std::map<std::string, std::string> m_scripts;
    
    bool checkSDCard();
    bool loadSingleScript(const std::string &appName, const std::string& appId);
};