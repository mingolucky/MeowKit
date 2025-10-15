/**
 * @file SDMMC_SD.hpp
 * @author 
 * @brief ESP32-S3 SDMMC 1-bit 3-wire SD card driver
 * @version 0.1
 * @date 
 * 
 * @copyright Copyright (c) 
 * 
 */
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include "../config.h"

class SDMMC_SD {
private:
    bool _inited;
    bool _last_status;  // Record the last status to detect status changes

public:
    SDMMC_SD();
    ~SDMMC_SD();

    /**
     * @brief Initialize SDMMC 1-bit mode
     * @return true success
     * @return false fail
     */
    bool init();
    bool isInited() const;
    
    /**
     * @brief Detect whether the SD card exists (dynamic detection)
     * @return true SD card present
     * @return false SD card does not exist
     */
    bool checkCardPresent();
    
    /**
     * @brief Reinitialize the SD card
     * @return true success
     * @return false fail
     */
    bool reinit();
    
    /**
     * @brief Get SD card information
     * @return String SD card information string
     */
    String getCardInfo();
};
