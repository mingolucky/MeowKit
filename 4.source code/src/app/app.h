#pragma once
#include "ble_beacon/ble_beacon.h"
#include "wifi_spam/wifi_spam.h"
#include "../bsp/config.h"
#if ENABLE_BADUSB_APP
#include "bad_usb/bad_usb.h"
#endif
#include "app_template/app_template.h"
#include "led/led.h"
#include "air_mouse/air_mouse.h"
#include "game/dino.h"
#include "pc_monitor/pc_monitor.h"
#include "vu_meter/vu_meter.h"
#include "retro_tv/retro_tv.h"
#include "music/music.h"

#include <mooncake.h>
#include <memory>
#if __cplusplus <= 201103L
namespace std
{
    template <typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args &&...args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif

/* Header files locator(Don't remove) */

inline void MeowKit_app_install_callback(mooncake::Mooncake *mooncake, DEVICES *device)
{
    /* Install app locator(Don't remove) */
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::BleBeacon>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::WiFiSpam>(device));
#if ENABLE_BADUSB_APP
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::BadUSB>(device));
#endif
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::LED>(device));

    mooncake->installApp(std::make_unique<MOONCAKE::APPS::AirMouse>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::Dino>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::MUSIC>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::PCMonitor>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::VUMeter>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::RetroTV>(device));

    mooncake->installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>(device));
    mooncake->installApp(std::make_unique<MOONCAKE::APPS::AppTemplate>(device));
}