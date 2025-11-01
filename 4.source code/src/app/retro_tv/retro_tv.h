/**
 * @file retro_tv.h
 * @author Mingo
 * @brief RetroTV app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <mooncake.h>
#include "../../MeowKit.h"
#include <WiFi.h>
#include "ScreenShotReceiver/TCPReceiver.h"
#include "../../bsp/devices.h"
#include <iostream>

class TCPReceiver;

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief RetroTV app
     */
    class RetroTV : public AppAbility
    {
    public:
        RetroTV(DEVICES *device);
        ~RetroTV();
        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        DEVICES *_device = nullptr;
        TCPReceiver *_recv = nullptr; // The receiver as a member variable
        bool _wifiConnected = false;  // WiFi connection status
        bool _isClosing = false;      // Added closed status flag
    };
}