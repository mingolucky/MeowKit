/**
 * @file badusb.h
 * @author Mingo
 * @brief BadUSB app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include "../../MeowKit.h"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief LED应用
     */
    class LED : public AppAbility {
    public:
        LED(DEVICES* device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;
    private:
        DEVICES* _device = nullptr;
    // Direct Arduino GPIO for LED (negative side connected to this pin; positive to 3.3V)
    #define LED_PIN 8

        enum Effect {
            EFFECT_OFF = 0,        // Extinguished
            EFFECT_ON,             // Constantly on
            EFFECT_BLINK_SLOW,     // Slow flash 1Hz
            EFFECT_BLINK_FAST,     // Flash 5Hz
            EFFECT_BREATHE,        // breathe
            EFFECT_PULSE,          // Double pulse heartbeat
            EFFECT_STROBE,         // Strobe
            EFFECT_CANDLE,         // Candle flicker (random jitter)
            EFFECT_SOS             // Morse SOS
        };

    Effect currentEffect = EFFECT_BREATHE;
    unsigned long lastToggle = 0;
    unsigned long blinkInterval = 500; // Default flash interval (will be adjusted according to the effect)

        // Breathe params
        unsigned long breatheStart = 0;
        unsigned long pwmPeriod = 20; // ms base PWM period for software PWM
        bool ledState = false; // current physical state (true means LED lit)

    // Universal Effects Timing
    unsigned long effectStart = 0; // Current effect start time

    // Candlelight effect
    int candleDuty = 60;                 // 0..100
    unsigned long candleNextUpdate = 0;  // Next random update time
    // SOS State Machine
    const char* sosPattern = "...---..."; // S O S
    size_t sosIndex = 0;                  // Current symbol index
    bool sosPhaseOn = false;              // Is the current dot/dash point displayed?
    unsigned long sosStepEnd = 0;         // Current phase end timestamp
    };
}
