/**
 * @file vu_meter.cpp
 * @author Mingo
 * @brief VU Meter app implementation for Mooncake
 * @version 0.2
 * @date 2025-08-08
 * @copyright Copyright (c) 2025
 */
#include "vu_meter.h"
#include <cmath>
#include <driver/i2s.h>
#include "../../bsp/devices.h"

namespace
{

    // Sampling parameters - Optimizing real-time performance
    constexpr int kSampleRate = 44100;
    constexpr int kBitsPerSample = I2S_BITS_PER_SAMPLE_16BIT;
    constexpr int kChannels = 2;
    constexpr size_t kChunkSamples = 256; // FFT-friendly size
    constexpr size_t kDmaBufLen = 512;
    constexpr int kDmaBufCount = 4;

    // VU Mapping and Smoothing - Further optimized jitter
    constexpr float kDbFloor = -65.0f;     // Expand bottom range and improve sensitivity
    constexpr float kDbCeil = -25.0f;      // Adjust the top range to avoid oversaturation
    constexpr float kAttackAlpha = 0.15f;  // Further reduce attack speed and reduce sudden jumps
    constexpr float kReleaseAlpha = 0.08f; // Smoother release
    constexpr float kMinRms = 1.0f;

    // Pointer Angle Range - Changed to -55° to +55° (110° total range)
    constexpr float kAngleMin = -55.0f; // Starting from -55°
    constexpr float kAngleMax = 55.0f;  // Ends at +55°

    // Audio preprocessing parameters - Enhanced silence detection
    constexpr float kNoiseGate = 30.0f;        // Increase the threshold value and reduce the sensitivity to ambient noise
    constexpr float kSilenceThreshold = 50.0f; // Silence detection threshold
    constexpr uint32_t kSilenceTimeMs = 500;   // Silence duration, after which the temperature is locked to -60°

    // I2S pin configuration
    constexpr i2s_pin_config_t kI2sPins = {
        .mck_io_num = HAL_PIN_I2S_MCLK, // 38
        .bck_io_num = HAL_PIN_I2S_BCLK, // 14
        .ws_io_num = HAL_PIN_I2S_WS,    // 13
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = HAL_PIN_I2S_DIN // 12
    };

    inline i2s_config_t makeInputI2SConfig()
    {
        i2s_config_t c{};
        c.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
        c.sample_rate = kSampleRate;
        c.bits_per_sample = static_cast<i2s_bits_per_sample_t>(kBitsPerSample);
        c.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
        c.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        c.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
        c.dma_buf_count = kDmaBufCount;
        c.dma_buf_len = kDmaBufLen;
        c.use_apll = true;
        c.tx_desc_auto_clear = false;
        c.fixed_mclk = 0;
#if ESP_IDF_VERSION_MAJOR >= 5
        c.mclk_multiple = I2S_MCLK_MULTIPLE_256;
        c.bits_per_chan = I2S_BITS_PER_CHAN_16BIT;
#endif
        return c;
    }

    static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
    {
        if (in_max == in_min)
            return out_min;
        float mapped = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        return mapped;
    }

    static float constrainFloat(float x, float a, float b)
    {
        return x < a ? a : (x > b ? b : x);
    }

    // Simplified amplitude calculation - replaces FFT and enhances silence detection
    static float calculateAmplitude(const int16_t *buffer, size_t samples)
    {
        double sum = 0.0;
        double sumSquared = 0.0;
        size_t validSamples = 0;
        float maxSample = 0.0f;

        for (size_t i = 0; i < samples; i++)
        {
            float sample = static_cast<float>(buffer[i]);
            float absSample = std::abs(sample);

            // Record the maximum sample value
            if (absSample > maxSample)
            {
                maxSample = absSample;
            }

            if (absSample > kNoiseGate)
            {
                sum += absSample;
                sumSquared += sample * sample;
                validSamples++;
            }
        }

        // If the maximum sample is lower than the silence threshold, return 0 directly
        if (maxSample < kSilenceThreshold)
        {
            return 0.0f;
        }

        if (validSamples == 0)
            return 0.0f;

        // Use RMS value as amplitude
        float rms = std::sqrt(sumSquared / validSamples);
        return rms;
    }

} // namespace

namespace MOONCAKE::APPS
{
    VUMeter::VUMeter(DEVICES *device)
        : _device(device), _silenceStartTime(0), _inSilence(false)
    {
        setAppInfo().name = "VUMeter";
    }

    void VUMeter::setupPointerPivot_()
    {
        if (!ui_line)
        {
            return;
        }

        // Make sure the object is fully loaded
        lv_obj_update_layout(ui_line);
        lv_refr_now(lv_disp_get_default());
        delay(50);

        // Get size
        lv_coord_t w = lv_obj_get_width(ui_line);
        lv_coord_t h = lv_obj_get_height(ui_line);
        lv_coord_t x = lv_obj_get_x(ui_line);
        lv_coord_t y = lv_obj_get_y(ui_line);

        // Make sure objects are visible and actionable
        lv_obj_clear_flag(ui_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_line, LV_OBJ_FLAG_CLICKABLE);

        // Set the pivot point - in the center of the bottom of the pointer
        lv_coord_t pivot_x = w / 2;
        lv_coord_t pivot_y = h - 8; // Slightly adjust upwards
        lv_img_set_pivot(ui_line, pivot_x, pivot_y);

        // Set the initial angle
        _angleDeg = 0.0f;
        lv_img_set_angle(ui_line, 0);
    }

    void VUMeter::setPointerAngle_(float deg)
    {
        if (!ui_line)
        {
            return;
        }

        // Constrained angle range (-60° to +60°)
        deg = constrainFloat(deg, kAngleMin, kAngleMax);

        // Check angle changes to avoid invalid updates and increase thresholds to reduce jitter
        static float lastAngle = -999.0f;
        if (std::abs(deg - lastAngle) < 1.0f)
        {           // Increase the threshold to 1 degree
            return; // The angle change is too small, skipping the update
        }

        // The LVGL angle unit is 0.1 degrees, and a positive value is clockwise.
        int16_t lvgl_angle = static_cast<int16_t>(deg * 10.0f);

        // Setting the angle
        lv_img_set_angle(ui_line, lvgl_angle);

        // Reduce refresh operations to avoid ghosting
        lv_obj_invalidate(ui_line);

        _angleDeg = deg;
        lastAngle = deg;
    }

    bool VUMeter::initAudio_()
    {
        if (!_device)
        {
            return false;
        }

        // 1) Initializing the ES7210 ADC
        esp_err_t err = _device->es7210.begin(ES7210_44K_SAMPLES, ES7210_16_BITS, ES7210_I2S_NORMAL, ES7210_MODE_SLAVE);
        if (err != ESP_OK)
        {
            return false;
        }

        _device->es7210.selectMic(static_cast<es7210_input_mics_t>(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2));
        _device->es7210.setGain(GAIN_15DB); // Maximum gain improves sensitivity

        err = _device->es7210.start();
        if (err != ESP_OK)
        {
            return false;
        }

        // 2) Initialize I2S
        auto i2sCfg = makeInputI2SConfig();
        err = i2s_driver_install(_i2sPort, &i2sCfg, 0, nullptr);
        if (err != ESP_OK)
        {
            return false;
        }

        err = i2s_set_pin(_i2sPort, &kI2sPins);
        if (err != ESP_OK)
        {
            i2s_driver_uninstall(_i2sPort);
            return false;
        }

        err = i2s_set_clk(_i2sPort, kSampleRate, static_cast<i2s_bits_per_sample_t>(kBitsPerSample), I2S_CHANNEL_STEREO);
        if (err != ESP_OK)
        {
            // Continue even if clock config fails
        }

        i2s_zero_dma_buffer(_i2sPort);

        err = i2s_start(_i2sPort);
        if (err != ESP_OK)
        {
            i2s_driver_uninstall(_i2sPort);
            return false;
        }

        _emaDb = -90.0f;
        _lastSample = 0.0f;
        _audioReady = true;

        return true;
    }

    void VUMeter::deinitAudio_()
    {
        if (_device)
        {
            _device->es7210.stop();
        }

        if (_audioReady)
        {
            i2s_stop(_i2sPort);
            i2s_driver_uninstall(_i2sPort);
        }

        _audioReady = false;
    }

    void VUMeter::onOpen()
    {
        // Loading UI
        ui_VU_Meter_screen_init();
        lv_scr_load_anim(ui_VU_Meter, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);

        // Wait for the UI to fully load
        lv_refr_now(lv_disp_get_default());
        delay(300);

        if (!ui_line)
        {
            return;
        }

        // Setting the pointer
        setupPointerPivot_();

        // Continuous oscillation test - -60° to +60°
        for (int cycle = 0; cycle < 1; cycle++)
        { // Reduce the number of cycles
            for (int deg = -60; deg <= 60; deg += 5)
            { // Increase step size to reduce ghosting
                setPointerAngle_(deg);
                lv_refr_now(lv_disp_get_default());
                delay(50); // Increase delay
            }
            for (int deg = 60; deg >= -60; deg -= 5)
            {
                setPointerAngle_(deg);
                lv_refr_now(lv_disp_get_default());
                delay(50);
            }
        }

        setPointerAngle_(0.0f);

        // Initialize audio
        if (!initAudio_())
        {
            return;
        }
    }

    void VUMeter::updateVu_()
    {
        if (!_audioReady || !ui_line)
            return;

        static int16_t buffer[kChunkSamples * kChannels];
        static uint32_t lastRefreshTime = 0;
        size_t bytesRead = 0;

        // Reading audio data
        esp_err_t err = i2s_read(_i2sPort, buffer, sizeof(buffer), &bytesRead, 1);

        if (err != ESP_OK || bytesRead == 0)
        {
            // Slowly returns to center position (0°) when muted
            _emaDb = _emaDb * 0.98f + (-90.0f) * 0.02f;
            float angle = mapFloat(_emaDb, kDbFloor, kDbCeil, kAngleMin, kAngleMax);
            angle = constrainFloat(angle, kAngleMin, kAngleMax);
            setPointerAngle_(angle);
            // Reduce refresh rate to avoid ghosting
            uint32_t currentTime = millis();
            if (currentTime - lastRefreshTime > 50)
            { // 20FPS refresh
                lv_refr_now(lv_disp_get_default());
                lastRefreshTime = currentTime;
            }
            return;
        }

        const size_t samples = bytesRead / sizeof(int16_t);
        if (samples < 4)
            return;

        // Calculate audio amplitude (alternative to FFT)
        float amplitude = calculateAmplitude(buffer, samples);

        uint32_t currentTime = millis();

        if (amplitude > 0)
        {
            // Detects a valid audio signal and exits the silent state
            if (_inSilence)
            {
                _inSilence = false;
            }
            _silenceStartTime = 0; // Reset the Quiet Timer

            // Convert to dBFS
            double dbfs = 20.0 * std::log10((amplitude + kMinRms) / 32768.0);
            dbfs = std::max(-90.0, std::min(0.0, dbfs));

            // Gentle smoothing
            float alpha = (dbfs > _emaDb) ? kAttackAlpha : kReleaseAlpha;
            _emaDb += alpha * (static_cast<float>(dbfs) - _emaDb);

            // Mapped to an angle range of -60° to +60°
            float angle = mapFloat(_emaDb, kDbFloor, kDbCeil, kAngleMin, kAngleMax);
            angle = constrainFloat(angle, kAngleMin, kAngleMax);

            // Update pointer
            setPointerAngle_(angle);

            // Reduce refresh rate to reduce ghosting
            if (currentTime - lastRefreshTime > 33)
            { // About 30FPS refresh rate
                lv_refr_now(lv_disp_get_default());
                lastRefreshTime = currentTime;

                // The serial port printing frequency is consistent with the pointer refresh frequency
                Serial0.printf("[VU] Amplitude: %.0f dB: %.1f\n", amplitude, _emaDb);
            }
        }
        else
        {
            // Silence detected
            if (!_inSilence)
            {
                if (_silenceStartTime == 0)
                {
                    _silenceStartTime = currentTime;
                }
                else if (currentTime - _silenceStartTime > kSilenceTimeMs)
                {
                    // Enter silent mode
                    _inSilence = true;

                    // Smoothly moves the pointer to the -60° position
                    _emaDb = kDbFloor;
                    setPointerAngle_(kAngleMin);
                    lv_refr_now(lv_disp_get_default());
                }
            }
            else
            {
                // Already in silent mode, keep the pointer at -60°
                // Do not update anything to avoid jitter
            }
        }
    }

    void VUMeter::onRunning()
    {
        updateVu_();

        // Added test mode: if you long press a button, it will enter the automatic swing test
        static uint32_t testModeStart = 0;
        static bool testMode = false;

        // Check whether to enter test mode (this can be replaced with actual key detection)
        if (_device && millis() - testModeStart > 10000 && !testMode)
        { // Enter the test in 10 seconds
            testMode = true;
        }

        // Test mode: More natural swing, simulating a real VU meter
        if (testMode)
        {
            static uint32_t lastTestUpdate = 0;
            static float testAngle = 0.0f;
            static float testDirection = 1.0f;

            uint32_t currentTime = millis();
            if (currentTime - lastTestUpdate > 100)
            {                                      // Reduce to 10Hz update, more natural
                testAngle += testDirection * 1.5f; // Reduce step size

                if (testAngle >= 60.0f)
                { // Changed to +60° range
                    testAngle = 60.0f;
                    testDirection = -1.0f;
                }
                else if (testAngle <= -60.0f)
                { // Changed to -60° range
                    testAngle = -60.0f;
                    testDirection = 1.0f;
                }

                setPointerAngle_(testAngle);
                // Reduce the refresh rate of the test pattern
                if (currentTime % 200 == 0)
                { // Refresh every 200ms
                    lv_refr_now(lv_disp_get_default());
                }

                lastTestUpdate = currentTime;
            }
        }
    }

    void VUMeter::onClose()
    {
        deinitAudio_();

        if (_device && _device->Lcd.width() > 0 && _device->Lcd.height() > 0)
        {
            _device->Lcd.fillScreen(TFT_BLACK);
        }
    }
}