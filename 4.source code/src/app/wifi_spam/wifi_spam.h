/**
 * @file wifi_spam.h
 * @author Mingo
 * @brief WiFi Spam app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "asset/wifi_spam_png.h"
#include "../../MeowKit.h"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief WiFi Spam - WiFi Network Jammer
     */
    class WiFiSpam : public AppAbility
    {
    public:
        WiFiSpam(DEVICES *device);
        virtual ~WiFiSpam() = default;

        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        DEVICES *_device;
        int currentLine; // Current row counter

        void displayTextOnBackground(const char *text, bool newLine = true);
        void clearDisplayArea();
    };
}
