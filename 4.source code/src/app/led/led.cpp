/**
 * @file badusb.cpp
 * @author Mingo
 * @brief BadUSB app implementation for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */

#include "led.h"
#include <Arduino.h>

namespace MOONCAKE::APPS
{
    LED::LED(DEVICES* device)
        : _device(device)
    {
        setAppInfo().name = "LED";
    }

    void LED::onOpen()
    {
    if (!_device) return;

    // configure GPIO pin as output and set high (LED off for common-anode)
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    currentEffect = EFFECT_BREATHE; // default effect
    breatheStart = millis();
    effectStart = millis();
    sosIndex = 0; sosPhaseOn = false; sosStepEnd = 0;
    }

    

    void LED::onRunning()
    {
       if (!_device) return;

       // toggle effect with A (cycle through all effects)
       _device->button.A.read();
       if (_device->button.A.pressed()) {
           currentEffect = static_cast<Effect>((static_cast<int>(currentEffect) + 1) % (static_cast<int>(EFFECT_SOS) + 1));
           // reset per-effect timers
           effectStart = millis();
           breatheStart = effectStart;
           lastToggle = effectStart;
           sosIndex = 0; sosPhaseOn = false; sosStepEnd = 0;
       }

    // only A button is used to cycle effects; B is ignored

       unsigned long now = millis();

       switch (currentEffect) {
           case EFFECT_OFF: {
               digitalWrite(LED_PIN, HIGH); // off (high for common-anode)
               break;
           }
           case EFFECT_ON: {
               digitalWrite(LED_PIN, LOW); // on
               break;
           }
           case EFFECT_BLINK_SLOW: {
               blinkInterval = 500; // 1Hz -> 500ms on/off
               if (now - lastToggle >= blinkInterval) {
                   lastToggle = now;
                   ledState = !ledState;
                   digitalWrite(LED_PIN, ledState ? LOW : HIGH);
               }
               break;
           }
           case EFFECT_BLINK_FAST: {
               blinkInterval = 100; // 5Hz -> 100ms on/off
               if (now - lastToggle >= blinkInterval) {
                   lastToggle = now;
                   ledState = !ledState;
                   digitalWrite(LED_PIN, ledState ? LOW : HIGH);
               }
               break;
           }
           case EFFECT_BREATHE: {
               // triangle wave breathe, period 2000ms
               const unsigned long period = 2000;
               unsigned long t = (now - breatheStart) % period;
               float phase = (t < period/2) ? (float)t / (period/2) : (float)(period - t) / (period/2);
               int duty = (int)(phase * 100.0f); // 0..100
               unsigned long pwmPhase = now % pwmPeriod;
               digitalWrite(LED_PIN, (pwmPhase < (pwmPeriod * duty / 100)) ? LOW : HIGH);
               break;
           }
           case EFFECT_PULSE: {
               // Double pulse (heartbeat): on 70ms, off 80ms, on 70ms, off 600ms
               unsigned long t = (now - effectStart) % 820UL; // 总周期 ~820ms
               bool on = (t < 70) || (t >= 150 && t < 220);
               digitalWrite(LED_PIN, on ? LOW : HIGH);
               break;
           }
           case EFFECT_STROBE: {
               // Strobe: 10ms on, 40ms off
               unsigned long t = (now - effectStart) % 50UL;
               bool on = (t < 10);
               digitalWrite(LED_PIN, on ? LOW : HIGH);
               break;
           }
           case EFFECT_CANDLE: {
               //Candlelight: Random Duty Jitter and Short-Period PWM
               if (now >= candleNextUpdate) {
                   // Randomly jumps in the range of 40..90 with small random steps
                   int step = random(-8, 9);
                   candleDuty = constrain(candleDuty + step, 40, 95);
                   candleNextUpdate = now + (unsigned long)random(30, 120); // 30-120ms renew
               }
               unsigned long pwmPhase = now % pwmPeriod; // 20ms Basic cycle
               digitalWrite(LED_PIN, (pwmPhase < (pwmPeriod * candleDuty / 100)) ? LOW : HIGH);
               break;
           }
           case EFFECT_SOS: {
               // Morse: dot (200ms on, 200ms off); dash (600ms on, 200ms off); additional 400ms between characters
               // mode：... --- ...，cycle
               const unsigned long dot = 200, dash = 600, gap = 200, chGap = 400;
               if (now >= sosStepEnd) {
                   // Enter the next phase
                   if (!sosPhaseOn) {
                       // Click the seat
                       char sym = sosPattern[sosIndex];
                       if (sym == '.') sosStepEnd = now + dot;
                       else if (sym == '-') sosStepEnd = now + dash;
                       else { // Safety back-up, reset
                           sosIndex = 0; sym = sosPattern[0]; sosStepEnd = now + dot;
                       }
                       sosPhaseOn = true;
                   } else {
                       // Extinguishing phase (basic interval after dot/dash)
                       sosStepEnd = now + gap;
                       sosPhaseOn = false;
                       // If a symbol has just ended, advance to the next one; handle extra space between characters
                       sosIndex = (sosPattern[sosIndex + 1] == '\0') ? 0 : sosIndex + 1;
                       if (sosIndex == 0) {
                           // A complete SOS ends, adding character spacing
                           sosStepEnd = now + chGap; // Extending the basic gap
                       }
                   }
               }
               digitalWrite(LED_PIN, sosPhaseOn ? LOW : HIGH);
               break;
           }
       }
    }

             

    void LED::onClose()
    {
    if (!_device) return;
    // ensure LED off
    digitalWrite(LED_PIN, HIGH);
    _device->Lcd.fillScreen(TFT_BLACK);
    }
}
