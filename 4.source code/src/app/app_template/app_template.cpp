/**
 * @file app_template.cpp
 * @author Mingo
 * @brief App template implementation for Mooncake
 * @version 0.1
 * @date 2025-06-03
 * @copyright Copyright (c) 2025
 */

#include "app_template.h"
#include <iostream>

namespace MOONCAKE::APPS
{
    AppTemplate::AppTemplate(DEVICES *device)
        : _device(device)
    {
        setAppInfo().name = "AppTemplate";
    }

    void AppTemplate::onOpen()
    {
        std::cout << "AppTemplate::onOpen() called" << std::endl;

        _device->Lcd.fillScreen(TFT_BLACK);
        matrix_effect.init(&_device->Lcd);
        matrix_effect.setup(
            10 /* Line Min */,
            30, /* Line Max */
            5,  /* Speed Min */
            25, /* Speed Max */
            30 /* Screen Update Interval */);
    }

    void AppTemplate::onRunning()
    {
        matrix_effect.loop();
    }

    void AppTemplate::onClose()
    {
        std::cout << "AppTemplate::onClose() called" << std::endl;
        // Clear the screen to prevent ghosting
        if (_device && _device->Lcd.width() > 0 && _device->Lcd.height() > 0)
        {
            _device->Lcd.fillScreen(TFT_BLACK);
        }
    }
}
