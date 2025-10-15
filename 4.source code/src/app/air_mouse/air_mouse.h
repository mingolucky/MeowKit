/**
 * @file air_mouse.h
 * @author Mingo
 * @brief AirMouse app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <mooncake.h>
#include "../../MeowKit.h"
#include "asset/air_mouse_png.h"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    class AirMouse : public AppAbility {
    public:
        AirMouse(DEVICES* device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        void _updateIMUData();
        void _updateButtonStates();
        void _updateUI();
        void _setupUI();
        void _displayTextOnBackground(const char* text, bool newLine = true);
        void _clearDisplayArea();

    private:
        DEVICES* _device = nullptr;
        
        // IMU data and mouse status
        QMI8658A_Data _imuData;
        unsigned long _lastUpdateTime = 0;
        bool _mouseActive = false;
        bool _leftButtonPressed = false;
        bool _rightButtonPressed = false;
        
        // Performance optimization parameters
        static constexpr float MOUSE_SENSITIVITY = 1.0f;  // Improve sensitivity
        static constexpr unsigned long UPDATE_INTERVAL_MS = 15;  // Increase update frequency
        static constexpr unsigned long UI_UPDATE_INTERVAL_MS = 1000;  // UI update interval
        
        // UI display related
        unsigned long _lastUIUpdate = 0;
        uint32_t _packetsSent = 0;
        int _currentLine = 0;
    };
}
