/**
 * @file vu_meter.h
 * @author Mingo
 * @brief VU Meter app for Mooncake
 * @version 0.2
 * @date 2025-08-08
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <mooncake.h>
#include "../../MeowKit.h"
#include "asset/ui.h"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief VU Meter app - Real-time audio level meters
     */
    class VUMeter : public AppAbility
    {
    public:
        VUMeter(DEVICES *device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        bool initAudio_();
        void deinitAudio_();
        void updateVu_();
        void setPointerAngle_(float deg); // Image rotation method
        void setupPointerPivot_();

    private:
        DEVICES *_device = nullptr;

        // Audio acquisition/processing status
        bool _audioReady = false;
        float _emaDb = -90.0f;    // Smoothed dB value
        float _angleDeg = 0.0f;   // Current pointer angle
        float _lastSample = 0.0f; // High-pass filter status
        i2s_port_t _i2sPort = I2S_NUM_1;

        // Silent detection status
        uint32_t _silenceStartTime = 0; // Silence start time
        bool _inSilence = false;        // Is it in silent mode?
    };
}
