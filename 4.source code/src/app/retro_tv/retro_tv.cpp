/**
 * @file retro_tv.cpp
 * @author Mingo
 * @brief RetroTV app implementation for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#include "retro_tv.h"
#include "bsp/wifi/wifi_manager_bridge.h"

namespace MOONCAKE::APPS
{
    RetroTV::RetroTV(DEVICES *device)
        : _device(device), _recv(nullptr), _wifiConnected(false), _isClosing(false)
    {
        setAppInfo().name = "RetroTV";
    }

    RetroTV::~RetroTV()
    {
        std::cout << "RetroTV destructor called" << std::endl;

        // Ensure resources are cleaned up
        if (!_isClosing)
        {
            onClose();
        }
    }

    void RetroTV::onOpen()
    {
        std::cout << "RetroTV::onOpen() called" << std::endl;

        // Clear screen and set font style (same as devices.cpp)
        _device->Lcd.fillScreen(TFT_BLACK);
        _device->Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        _device->Lcd.setFont(&fonts::efontCN_16);

        int yPos = 10;       // Start Y position
        int lineHeight = 20; // Line height

        // Set Wi-Fi mode to STA，and obtain credentials from the provisioning network
        // Read the saved WiFi credentials (if none, do not block, prompt the user to go to the control center to configure the network)
        WiFi.mode(WIFI_STA);
        WiFi.persistent(true);
        WiFi.setAutoReconnect(true);

        char ssid[33] = {0};
        char password[65] = {0};
        if (!wifi_get_saved_credentials(ssid, sizeof(ssid), password, sizeof(password)) || ssid[0] == '\0')
        {
            _device->Lcd.setCursor(0, 0);
            _device->Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
            _device->Lcd.printf("No saved Wi-Fi found\nPlease configure in Control Center\n");
            _wifiConnected = false;
            return; // No more waiting, no more getting stuck
        }

        _device->Lcd.setCursor(0, 0);
        _device->Lcd.printf("Connecting to WiFi: %s\n", ssid);
        yPos += lineHeight;

        WiFi.begin(ssid, password);

        // Wait for connection with timeout (<= 10s)
        int connectAttempts = 0;
        const int maxAttempts = 20; // 10s timeout
        while (WiFi.status() != WL_CONNECTED && connectAttempts < maxAttempts)
        {
            delay(500);
            _device->Lcd.printf(".");
            connectAttempts++;
        }
        _device->Lcd.printf("\n");
        yPos += lineHeight;

        if (WiFi.status() == WL_CONNECTED)
        {
            // Success
            _device->Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
            _device->Lcd.setFont(&fonts::efontCN_16);
            _device->Lcd.printf("WiFi connected!\n");
            yPos += lineHeight;

            // Print IP label in larger, bold, white font
            _device->Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            _device->Lcd.setFont(&fonts::efontCN_24); // Increase font size
            _device->Lcd.printf("IP Address:\n");
            yPos += 24; // Font height

            // Print IP address in white, bold, large font
            _device->Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            _device->Lcd.setFont(&fonts::efontCN_24);
            _device->Lcd.printf("%s\n", WiFi.localIP().toString().c_str());
            yPos += 24;

            std::cout << "\nWiFi Connected. IP: " << WiFi.localIP().toString().c_str() << std::endl;
            _wifiConnected = true;

            // Restore normal font for following info
            _device->Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
            _device->Lcd.setFont(&fonts::efontCN_16);

            // Create TCPReceiver and setup
            try
            {
                _recv = new TCPReceiver();
                if (_recv)
                {
                    _recv->setup(&_device->Lcd);
                    _device->Lcd.printf("TCP receiver started\n");
                    std::cout << "TCPReceiver setup completed" << std::endl;
                }
                else
                {
                    _device->Lcd.setTextColor(TFT_RED, TFT_BLACK);
                    _device->Lcd.printf("TCP receiver create failed\n");
                    std::cout << "Failed to create TCPReceiver" << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                _device->Lcd.setTextColor(TFT_RED, TFT_BLACK);
                _device->Lcd.printf("TCP receiver exception\n");
                std::cout << "Exception creating TCPReceiver: " << e.what() << std::endl;
                _recv = nullptr;
            }
        }
        else
        {
            // Failed: prompt and return, no blocking
            _device->Lcd.setTextColor(TFT_RED, TFT_BLACK);
            _device->Lcd.setFont(&fonts::efontCN_16);
            _device->Lcd.printf("Wi-Fi connection failed (10s timeout)\nPlease check router or reconfigure\n");
            _wifiConnected = false;
            return;
        }
    }

    void RetroTV::onRunning()
    {
        if (_wifiConnected && _recv)
        {
            try
            {
                _recv->loop();
            }
            catch (const std::exception &e)
            {
                std::cout << "Exception in TCPReceiver loop: " << e.what() << std::endl;
                // You can choose to recreate the receiver or display an error message
            }
        }
        else
        {
            // Display error status or retry the connection
            static uint32_t lastStatusUpdate = 0;
            if (millis() - lastStatusUpdate > 1000)
            {
                lastStatusUpdate = millis();

                _device->Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
                _device->Lcd.setFont(&fonts::efontCN_16);

                if (!_wifiConnected)
                {
                    _device->Lcd.fillRect(10, 140, 200, 20, TFT_BLACK);
                    _device->Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                    _device->Lcd.drawString("No WiFi connection", 10, 140);
                }
                else if (!_recv)
                {
                    _device->Lcd.fillRect(10, 140, 200, 20, TFT_BLACK);
                    _device->Lcd.setTextColor(TFT_ORANGE, TFT_BLACK);
                    _device->Lcd.drawString("TCP Receiver Error", 10, 140);
                }
            }
        }
    }

    void RetroTV::onClose()
    {
        if (_isClosing)
        {
            return; // Avoid repeated cleaning
        }

        _isClosing = true;
        std::cout << "RetroTV::onClose() called" << std::endl;

        try
        {
            // First stop the TCPReceiver activity
            if (_recv)
            {
                std::cout << "Cleaning up TCPReceiver..." << std::endl;

                // If there is a stop method, call it first
                // _recv->stop();

                delay(50);
                delete _recv;
                _recv = nullptr;
                std::cout << "TCPReceiver cleaned up" << std::endl;
            }

            // Handle WiFi disconnections gracefully
            if (_wifiConnected)
            {
                std::cout << "Disconnecting WiFi..." << std::endl;

                // Set to STA mode and disconnect
                WiFi.mode(WIFI_STA);
                delay(50);
                WiFi.disconnect(false); // false : Indicates not to clear the configuration
                delay(100);

                // Finally set to OFF mode
                WiFi.mode(WIFI_OFF);
                delay(50);

                _wifiConnected = false;
                std::cout << "WiFi disconnected safely" << std::endl;
            }

            // Clean up the display
            if (_device && _device->Lcd.width() > 0)
            {
                _device->Lcd.fillScreen(TFT_BLACK);
            }

            delay(100); // Give the system time to complete the cleanup

            std::cout << "RetroTV closed successfully" << std::endl;
        }
        catch (...)
        {
            std::cout << "Exception in RetroTV::onClose(), forcing cleanup" << std::endl;

            // Forced cleanup
            _recv = nullptr;
            _wifiConnected = false;
            WiFi.mode(WIFI_OFF);
        }

        _isClosing = false;
    }
}