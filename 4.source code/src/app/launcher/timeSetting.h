/**
 * @file launcher.h
 * @author Mingo
 * @brief
 * @version 0.1
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <WiFi.h>
#include <time.h>

class timeSetting
{
public:
    timeSetting();
    ~timeSetting();
    bool initTime();

    void setTime();

private:
    // Time zone setting: For example, China is UTC+8
    const char *ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 8 * 3600; // China is +8 hours
    const int daylightOffset_sec = 0;    // Daylight saving time offset, if any, set to 3600
};
