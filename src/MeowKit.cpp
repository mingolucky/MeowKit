/**
 * @file MeowKit.cpp
 * @brief MeowKit application entry — manages device lifecycle
 */
#include "MeowKit.h"
#include "splash/splash_screen.h"
#include "system/settings_bridge.h"
#include "system/system_sound.h"
#include "system/system_sound_assets.h"
#include "system/lua_host/lua_host.hpp"
#include "system/flash_mode/flash_mode.hpp"

bool MeowKit::Setup()
{
    _device = std::make_unique<DEVICES>();
    if (!_device) {
        printf("[MeowKit] BSP create failed\n");
        return false;
    }

    _device->init();

    // Phase 1 bring-up: confirm the embedded (PSRAM-backed) Lua VM runs on
    // hardware. Use Serial (USB-CDC) — plain printf() goes to UART0 and would
    // be invisible on the USB console. Remove once the App Runner is in place.
    Serial.printf("[LUA] %s\n", lua_host::selfTest().c_str());

    settings_init();
    sys_settings_bridge_attach(_device.get());
    system_sound_init(_device.get());
    settings_load_all();
    system_sound_play_boot(boot_animation_duration_ms);
    SplashScreen::show(_device->Lcd);
    system_sound_stop();

    _launcher = std::make_unique<Launcher>(_device.get());
    _launcher->onCreate();

    return true;
}

void MeowKit::Loop()
{
    // Dev convenience: lets the host drop the device into flash mode over the
    // USB console, no BOOT button required.
    flash_mode::poll(Serial);

    if (_launcher) {
        _launcher->onLoop();
    }
}
