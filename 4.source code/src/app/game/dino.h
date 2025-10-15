/**
 * @file dino.h
 * @author Mingo
 * @brief Dino app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <mooncake.h>
#include "../../MeowKit.h"
#include "include/Game.h"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief Game应用
     */
    class Dino : public AppAbility {
    public:
        Dino(DEVICES* device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;
    private:
        DEVICES* _device = nullptr;
    };
}