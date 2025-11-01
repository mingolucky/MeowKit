/**
 * @file dino.cpp
 * @author Mingo
 * @brief Dino app implementation for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#include "dino.h"
Game dino;
namespace MOONCAKE::APPS
{
    Dino::Dino(DEVICES *device)
        : _device(device)
    {
        setAppInfo().name = "Game";
    }

    void Dino::onOpen()
    {
        dino.begin(_device);
    }

    void Dino::onRunning()
    {
        dino.loop();
    }

    void Dino::onClose()
    {
        dino.~Game();
    }
}
