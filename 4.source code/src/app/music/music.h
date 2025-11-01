/**
 * @file music.h
 * @author Mingo
 * @brief BadUSB app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include "../../MeowKit.h"
#include <Arduino.h>
#include <Audio.h>
#include <SD_MMC.h>
#include <FS.h>
#include <vector>
#include "../../bsp/sd/sd.h"
#include "../../bsp/audio/es8311.h"
#include "../../bsp/config.h"
// UI assets (fonts/images)
#include "../../ui/src/ui.h"

#define PA_EN (1)

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief MUSIC app
     */
    class MUSIC : public AppAbility
    {
    public:
        MUSIC(DEVICES *device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        DEVICES *_device = nullptr;
        Audio _audio;
        std::vector<String> _musicFiles;
        int _currentIndex = 0;
        bool _isPlaying = false;
        bool _initialized = false;
        bool _paused = false;      // Is it paused?
        bool _userStarted = false; // The first click of A triggers the start of playback
        // Start the currently indexed song. Return true if successful and set _isPlaying
        bool startCurrent();
        // LVGL screen for MUSIC app
        lv_obj_t *_screen = nullptr;
        // UI objects
        lv_obj_t *_bg = nullptr;
        lv_obj_t *_btnAImg = nullptr;
        lv_obj_t *_btnBImg = nullptr;
        lv_obj_t *_btnALabel = nullptr;
        lv_obj_t *_btnBLabel = nullptr;
        lv_obj_t *_infoLabel = nullptr; // center text: filename + time
        uint32_t _lastUiUpdate = 0;
    };
}
