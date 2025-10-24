#pragma once

#include "bsp/devices.h"
#include <map>
#include <string>
#include <optional>

class luaScriptManager {
public:
    // 删除构造函数和赋值操作符
    luaScriptManager(const luaScriptManager&) = delete;
    luaScriptManager& operator=(const luaScriptManager&) = delete;
    
    // 获取单例实例
    static luaScriptManager& getInstance();
    
    // 从SD卡的script文件夹下读取所有以app_id开头的文件夹
    // 并从文件夹中读取以app_id.lua的脚本文件
    bool loadScriptsFromSD();
    
    // 运行指定ID的脚本
    bool getScriptById(const std::string& id, std::string& outScript) const;
    
    // 获取已加载的脚本列表
    const std::map<std::string, std::string>& getLoadedScripts() const;

    // 设置设备指针
    void setDevice(DEVICES* device);

private:
    luaScriptManager();  // 私有化构造函数
    ~luaScriptManager();

    DEVICES* m_device;
    std::map<std::string, std::string> m_scripts;
    
    bool checkSDCard();
    bool loadSingleScript(const std::string &appName, const std::string& appId);
};