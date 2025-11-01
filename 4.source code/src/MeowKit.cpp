/**
 * @file
 * @author
 * @brief
 * @version
 * @date
 *
 * @copyright
 *
 */
#include "MeowKit.h"

void MeowKit::Setup()
{
    printf("[MeowKit] Setup\n");
    /* Create bsp */
    _device = new DEVICES;
    if (_device == NULL)
    {
        printf("[MeowKit] bsp create failed\n");
    }

    /* Init device */
    _device->init();

    /* Create launcher */
    _launcher = std::unique_ptr<Launcher>(new Launcher(_device));
    _launcher->onCreate();
}

void MeowKit::Loop()
{
    _launcher->onLoop();
}

void MeowKit::Destroy()
{
    if (_device != nullptr)
    {
        delete _device;
        _device = nullptr;
    }
}

MeowKit::~MeowKit()
{
    Destroy();
}
