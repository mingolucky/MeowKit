#pragma once
#include "porting/touch.hpp"
#include "porting/display.hpp"
#include "button/button.hpp"
#include "imu/QMI8658A.h"
#include "sd/sd.h"
#include "rtc/pcf8563.h"
#include "config.h"
#include <PCA9557.h>
#include <BleMouse.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "audio/es8311.h"
#include "audio/es7210.h"
#include <Audio.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>

class DEVICES
{

public:
    CTP         ctp;
    Meow_LGFX   Lcd;
    Button      button;
    QMI8658A    imu;
    SDMMC_SD    sd;
    PCA9557     io;
    BleMouse    bleMouse;
    PCF8563     pcf;
    ES8311      es8311;
    ES7210      es7210;
    Audio       audio;
    // BLE Mouse Start flag to avoid reconnection exceptions caused by repeated startup
    bool        bleMouseStarted = false;
    

    DEVICES() : io(0x19, &Wire), 
                pcf(&Wire, PCF8563_ADDR), 
                es7210(&Wire, ES7210_ADDR),
                es8311(&Wire, ES8311_ADDR),
                // Setting the BLE device name
                bleMouse("MeowKit mouse", "MeowKit", 100)      
    {

    }
    
    ~DEVICES() {

    }
    
    /**
     * @brief Initialize the device
     * 
     */
    void init();
    int getBatteryPercent();
    
};