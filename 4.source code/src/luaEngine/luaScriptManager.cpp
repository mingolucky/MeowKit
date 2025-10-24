#include "luaScriptManager.h"
#include "luaTest.h"
#include "esp_log.h"
#include "FS.h"
#include "SD_MMC.h"

static const char *TAG = "luaScriptManager";

luaScriptManager::luaScriptManager() : m_device(nullptr)
{
}

luaScriptManager::~luaScriptManager()
{
}

luaScriptManager &luaScriptManager::getInstance()
{
    static luaScriptManager instance;
    return instance;
}

void luaScriptManager::setDevice(DEVICES *device)
{
    m_device = device;
}

bool luaScriptManager::checkSDCard()
{
    if (m_device == nullptr)
    {
        ESP_LOGE(TAG, "Device is not set");
        return false;
    }

    // Check if SD card is initialized
    if (!m_device->sd.isInited())
    {
        if (!m_device->sd.init())
        {
            ESP_LOGE(TAG, "SD card init failed");
            return false;
        }
    }
    return true;
}

bool luaScriptManager::loadScriptsFromSD()
{
    if (!checkSDCard())
    {
        ESP_LOGE(TAG, "SD card not initialized");
        return false;
    }

    // 清空之前加载的脚本
    m_scripts.clear();

    // 打开/script目录
    File scriptDir = SD_MMC.open("/script");
    if (!scriptDir || !scriptDir.isDirectory())
    {
        ESP_LOGE(TAG, "Failed to open /script directory");
        return false;
    }

    // 遍历/script目录下的所有子目录
    File file = scriptDir.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            std::string dirName = file.name();
            // 获取目录名作为app_id (例如: /script/app_1/app_1.lua)
            size_t lastSlash = dirName.find_last_of("/");
            std::string dirPart = (lastSlash != std::string::npos) ? dirName.substr(lastSlash + 1) : dirName;
            
            // 从最后一个下划线后提取数字作为app_id
            size_t lastUnderscore = dirPart.find_last_of("_");
            std::string appId = (lastUnderscore != std::string::npos) ? dirPart.substr(lastUnderscore + 1) : dirPart;

            // 加载该app_id对应的脚本
            loadSingleScript(dirPart,appId);
        }
        file = scriptDir.openNextFile();
    }

    scriptDir.close();
    return true;
}

bool luaScriptManager::loadSingleScript(const std::string &appName, const std::string &appId)
{
    // 构造脚本文件路径 /script/{appId}/{appId}.lua
    std::string scriptPath = "/script/" + appName + "/" + "main_" + appId + ".lua";
    ESP_LOGI(TAG, "Loading script from path: %s", scriptPath.c_str());

    // 打开脚本文件
    File file = SD_MMC.open(scriptPath.c_str(), FILE_READ);
    if (!file)
    {
        ESP_LOGE(TAG, "Failed to open Lua script file: %s", scriptPath.c_str());
        return false;
    }

    // 读取文件内容
    size_t fileSize = file.size();
    char *content = (char *)malloc(fileSize + 1);
    if (!content)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for Lua script");
        file.close();
        return false;
    }

    size_t bytesRead = file.read((uint8_t *)content, fileSize);
    file.close();

    if (bytesRead != fileSize)
    {
        ESP_LOGE(TAG, "Failed to read complete Lua script file");
        free(content);
        return false;
    }

    content[fileSize] = '\0';

    // 存储到map中
    m_scripts[appId] = std::string(content);
    free(content);

    ESP_LOGI(TAG, "Successfully loaded script for app_id: %s, size: %d bytes", appId.c_str(), fileSize);
    return true;
}

bool luaScriptManager::getScriptById(const std::string &id, std::string &outScript) const
{
    auto it = m_scripts.find(id);
    if (it == m_scripts.end())
    {
        ESP_LOGW(TAG, "Script with id %s not found", id.c_str());
        return false;
    }
    outScript = it->second;
    return true;
}

const std::map<std::string, std::string> &luaScriptManager::getLoadedScripts() const
{
    return m_scripts;
}