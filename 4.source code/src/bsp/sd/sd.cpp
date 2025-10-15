#include "sd.h"

SDMMC_SD::SDMMC_SD() : _inited(false), _last_status(false) {}

SDMMC_SD::~SDMMC_SD() {
    if (_inited) {
        SD_MMC.end();
    }
}

bool SDMMC_SD::init() {
    // Setting the SDMMC pins
    if (!SD_MMC.setPins(SDMMC_SD_PIN_CLK, SDMMC_SD_PIN_CMD, SDMMC_SD_PIN_D0)) {
        Serial0.println("[SDMMC_SD] setPins failed");
        return false;
    }

    // Try initializing the SD card
    if (SD_MMC.begin("/sdcard", true)) { // 1-bit mode
        _inited = true;
        _last_status = true;
        Serial0.println("SD card initialization successful");
        return true;
    } else {
        _inited = false;
        _last_status = false;
        Serial0.println("SD card initialization failed");
        return false;
    }
}

bool SDMMC_SD::isInited() const {
    return _inited;
}

bool SDMMC_SD::checkCardPresent() {
    // Check if the SD card exists
    uint8_t cardType = SD_MMC.cardType();
    bool current_status = (cardType != CARD_NONE);
    
    // If the state changes, update the initialization state
    if (current_status != _last_status) {
        _last_status = current_status;
        if (current_status && !_inited) {
            // The SD card is inserted and not initialized. Try to reinitialize it.
            reinit();
        } else if (!current_status && _inited) {
            // SD card removed, update status
            _inited = false;
            Serial0.println("SD card removed");
        }
    }
    
    return current_status && _inited;
}

bool SDMMC_SD::reinit() {
    if (_inited) {
        SD_MMC.end();
        _inited = false;
        delay(100); // Waiting for resource release
    }
    
    return init();
}

String SDMMC_SD::getCardInfo() {
    if (!_inited) {
        return "SD card not initialized";
    }
    
    uint8_t cardType = SD_MMC.cardType();
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    
    String info = "type: ";
    switch (cardType) {
        case CARD_NONE:
            info += "No card";
            break;
        case CARD_MMC:
            info += "MMC";
            break;
        case CARD_SD:
            info += "SD";
            break;
        case CARD_SDHC:
            info += "SDHC";
            break;
        default:
            info += "unknown(";
            info += String(cardType);
            info += ")";
            break;
    }
    
    info += ", capacity: " + String(cardSize) + "MB";
    return info;
}