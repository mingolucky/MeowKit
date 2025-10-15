/**
 * @file launcher.h
 * @author Mingo
 * @brief
 * @version 0.1
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <string>
#include <cstdint>
#include <mooncake.h>
#include "bsp/devices.h"

using namespace mooncake;

extern int selected_app_index;

namespace MOONCAKE
{
    namespace APPS
    {
        class Launcher
        {
        public:
             Launcher(DEVICES* device) : _device(device) {}
             void onCreate();
             void onLoop();
             void onDestroy();

        private:
            void reInitUI(int pageIndex);
            void updateDeviceStatus();
            void updateAPPManager();
            void sleep_mode();
            void ui_clock_update();

        private:
            
            std::unique_ptr<Mooncake> _mooncake = nullptr;
            DEVICES *_device = nullptr;
            bool _lvgl_enable = false;
        };
    }
} // namespace MOONCAKE::APPS