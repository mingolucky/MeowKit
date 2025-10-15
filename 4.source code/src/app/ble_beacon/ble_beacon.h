/**
 * @file ble_beacon.h
 * @author Mingo
 * @brief BLE Beacon app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include "../../MeowKit.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_arduino_version.h>
#include "asset/ble_beacon_bg_png.h"

// Bluetooth maximum transmit power
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21  // ESP32C3 ESP32C2 ESP32S3
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define MAX_TX_POWER ESP_PWR_LVL_P20  // ESP32H2 ESP32C6
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9   // Default
#endif

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief BLE Beacon
     */
    class BleBeacon : public AppAbility {
    public:
        BleBeacon(DEVICES* device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;
        
    private:
        void displayTextOnBackground(const char* text, bool newLine = true);
        void clearDisplayArea();

    private:
        DEVICES* _device;
        BLEAdvertising* pAdvertising;
        BLEServer* pServer;
        int delayMilliseconds;
        bool isInitialized;
        int currentLine;  // Add current row counter
    };
}
