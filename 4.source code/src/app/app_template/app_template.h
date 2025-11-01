/**
 * @file app_template.h
 * @author Mingo
 * @brief App template for Mooncake
 * @version 0.1
 * @date
 * @copyright Copyright (c) 2025
 */
#pragma once
#include "DigitalRainAnimation.hpp"
#include <mooncake.h>
#include "../../bsp/devices.h"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief AppTemplate Demo
     */
    class AppTemplate : public AppAbility
    {
    public:
        AppTemplate(DEVICES *device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        DEVICES *_device = nullptr;
        DigitalRainAnimation<Meow_LGFX> matrix_effect;
    };
}