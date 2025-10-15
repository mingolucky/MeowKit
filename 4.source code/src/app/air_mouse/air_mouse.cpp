/**
 * @file air_mouse.cpp
 * @author Mingo
 * @brief AirMouse app implementation for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#include "air_mouse.h"

namespace MOONCAKE::APPS
{
    AirMouse::AirMouse(DEVICES* device)
        : _device(device), _currentLine(0)
    {
        setAppInfo().name = "AirMouse";
    }

    void AirMouse::_displayTextOnBackground(const char* text, bool newLine)
    {
        if (!_device) return;

        // Text style: green foreground, transparent background
        _device->Lcd.setTextColor(TFT_GREEN);
        _device->Lcd.setFont(&fonts::efontCN_16);

        // Positioning
        int lineHeight = 18;
        int startY = 10;
        int startX = 10;

        if (newLine) {
            _currentLine++;
            _device->Lcd.setCursor(startX, startY + (_currentLine - 1) * lineHeight);
        }

        _device->Lcd.print(text);
    }

    void AirMouse::_clearDisplayArea()
    {
        if (!_device) return;
        
        _currentLine = 0;
        _device->Lcd.fillScreen(TFT_BLACK);
        _device->Lcd.pushImage(0, 0, air_mouse.width, air_mouse.height, (uint16_t*)air_mouse.data);
    }

    void AirMouse::onOpen()
    {
        if (!_device) return;

        // Show background image
        _clearDisplayArea();
        
        _displayTextOnBackground("AirMouse starting...");

        // Initialize IMU
        if (!_device->imu.begin()) {
            _displayTextOnBackground("IMU init failed");
            delay(800);
            return;
        }
        _displayTextOnBackground("IMU ready");

        // Initialize BLE mouse (begin once)
        if (!_device->bleMouseStarted) {
            _device->bleMouse.begin();
            _device->bleMouseStarted = true;
        }
        _displayTextOnBackground("BLE mouse ready");
        _displayTextOnBackground("Waiting for BLE connection...");

        // Calibrate gyro bias
        _displayTextOnBackground("Calibrating gyro...");
        _device->imu.calibrateGyroBias(100);
        _displayTextOnBackground("Calibration done");

        _displayTextOnBackground("Status: Idle (disconnected)");
        _displayTextOnBackground("A: Left click");
        _displayTextOnBackground("  B: Right click", false);
        
        // Only start running when BLE is connected
        _mouseActive = false;
        _lastUpdateTime = millis();
        _lastUIUpdate = millis();
    }

    void AirMouse::onRunning()
    {
        // Connection state machine: only runs core logic after connection
        bool connected = _device->bleMouse.isConnected();

        if (connected && !_mouseActive) {
            _mouseActive = true;
            _leftButtonPressed = false;
            _rightButtonPressed = false;
            _lastUpdateTime = millis();
            _displayTextOnBackground("BLE connected");
        }
        if (!connected && _mouseActive) {
            // Release all buttons to stop operation
            _device->bleMouse.release(MOUSE_LEFT);
            _device->bleMouse.release(MOUSE_RIGHT);
            _leftButtonPressed = false;
            _rightButtonPressed = false;
            _mouseActive = false;
            _displayTextOnBackground("BLE disconnected");
        }

        // High-frequency updates of IMU and mouse data (only when connected and active)
        unsigned long currentTime = millis();
        if (_mouseActive && connected && (currentTime - _lastUpdateTime >= UPDATE_INTERVAL_MS)) {
            _lastUpdateTime = currentTime;
            
            // Update IMU data
            _updateIMUData();
            
            // Update the button status (need to tick first to generate single/double click events)
            _device->button.A.tick();
            _device->button.B.tick();
            _updateButtonStates();
            
            // Sending mouse data
            if (_device->bleMouse.isConnected() && 
                (_imuData.mouse_dx != 0 || _imuData.mouse_dy != 0)) {
                _device->bleMouse.move(_imuData.mouse_dx, _imuData.mouse_dy);
                _packetsSent++;
            }
        }
        
        // Update UI display at low frequency (refresh status regardless of connection)
        if (currentTime - _lastUIUpdate >= UI_UPDATE_INTERVAL_MS) {
            _updateUI();
            _lastUIUpdate = currentTime;
        }
    }

    void AirMouse::onClose()
    {
        // Stop activity
        _mouseActive = false;

        // Release mouse buttons if connected
        if (_device && _device->bleMouse.isConnected()) {
            _device->bleMouse.release(MOUSE_LEFT);
            _device->bleMouse.release(MOUSE_RIGHT);
        }

        // reset state flags
        _leftButtonPressed = false;
        _rightButtonPressed = false;

        // NO screen updates, NO delays, silent safe exit
    }

    void AirMouse::_updateIMUData()
    {
        // Read and process IMU data
        if (_device->imu.readRawData(&_imuData)) {
            _device->imu.updateMouseDelta(&_imuData, MOUSE_SENSITIVITY);
        }
    }

    void AirMouse::_updateButtonStates()
    {
        if (!_device->bleMouse.isConnected()) return;
        
        // A button as left button (press/release to transfer, for dragging, etc.)
        if (_device->button.A.pressed() && !_leftButtonPressed) {
            _device->bleMouse.press(MOUSE_LEFT);
            _leftButtonPressed = true;
        } else if (_device->button.A.released() && _leftButtonPressed) {
            _device->bleMouse.release(MOUSE_LEFT);
            _leftButtonPressed = false;
        }

        // B button: Single click is mapped to a right click (press+release)
        if (_device->button.B.isSingleClick()) {
            _device->bleMouse.press(MOUSE_RIGHT);
            delay(20);
            _device->bleMouse.release(MOUSE_RIGHT);
        }
    }

    void AirMouse::_updateUI()
    {
        // Dynamic information display area
        int textStartY = 140;
        int textHeight = 80;
        
        // Redraw the dynamic information area of ​​the background image
        if (textStartY < air_mouse.height) {
            int copyHeight = min(textHeight, air_mouse.height - textStartY);
            _device->Lcd.pushImage(0, textStartY, air_mouse.width, copyHeight, 
                                 (uint16_t*)air_mouse.data + (textStartY * air_mouse.width));
        }
        
        // Set text style
        _device->Lcd.setTextColor(TFT_GREEN);
        _device->Lcd.setFont(&fonts::efontCN_16);
        
        // Display real-time information
        char buffer[64];
        int lineY = textStartY + 5;
        int lineSpacing = 16;
        
    // BLE connection status
    _device->Lcd.setCursor(10, lineY);
    sprintf(buffer, "BLE: %s", _device->bleMouse.isConnected() ? "Connected" : "Disconnected");
    _device->Lcd.print(buffer);
    lineY += lineSpacing;

    // Mouse movement
    _device->Lcd.setCursor(10, lineY);
    sprintf(buffer, "Move: X=%d Y=%d", _imuData.mouse_dx, _imuData.mouse_dy);
    _device->Lcd.print(buffer);
    lineY += lineSpacing;

    // Gyro (simplified)
    _device->Lcd.setCursor(10, lineY);
    sprintf(buffer, "Gyro: X=%.1f Y=%.1f", _imuData.gyr_x_dps, _imuData.gyr_y_dps);
    _device->Lcd.print(buffer);
    lineY += lineSpacing;

    // Stats
    _device->Lcd.setCursor(10, lineY);
    sprintf(buffer, "Sent: %d | Free: %dKB", _packetsSent, ESP.getFreeHeap() / 1024);
    _device->Lcd.print(buffer);
    }

    void AirMouse::_setupUI()
    {
        // Initialize the display
        _clearDisplayArea();
    }
}