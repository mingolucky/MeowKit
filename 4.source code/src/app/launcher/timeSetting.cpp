#include "timeSetting.h"
#include "bsp/wifi/wifi_manager_bridge.h"
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <ctime>
#include <iostream>
#include <WiFiManager.h>
#include "bsp/devices.h"
timeSetting::timeSetting()
{
}

timeSetting::~timeSetting()
{
}

bool timeSetting::initTime()
{
    // Get saved Wi-Fi information
    char ssid[64] = {0};
    char pass[64] = {0};

    if (wifi_get_saved_credentials(ssid, sizeof(ssid), pass, sizeof(pass)))
    {
        WiFi.begin(ssid, pass);
        // You can add logic to wait for connection or non-blocking check
        std::cout << "Wi-Fi saved" << std::endl;
    }
    else
    {
        std::cout << "Wi-Fi not saved" << std::endl;
        return false;
    }
    // Connect to Wi-Fi
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED)
    {
        // std::cout<<".";
    }
    std::cout << "Connected to Wi-Fi" << std::endl;
    // Configure NTP (will start the system SNTP client)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Wait for time synchronization (up to 10 seconds)
    std::cout << "Waiting for NTP time sync..." << std::endl;
    int retry = 0;
    const int retry_count = 20;
    while (retry < retry_count)
    {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            // Serial.println(&timeinfo, "Time synchronized: %Y-%m-%d %H:%M:%S");
            std::cout << "Time synchronized" << std::endl;

            extern DEVICES devices;

            // Set the system time
            RTC_Time rtc_time;
            rtc_time.hour = timeinfo.tm_hour;
            rtc_time.min = timeinfo.tm_min;
            rtc_time.sec = timeinfo.tm_sec;
            rtc_time.day = timeinfo.tm_mday;
            rtc_time.year = timeinfo.tm_year;
            rtc_time.month = timeinfo.tm_mon + 1;
            rtc_time.weekday = timeinfo.tm_wday;
            devices.pcf.setTime(rtc_time);

            return true;
        }
        retry++;
        // Serial.print(".");
    }
    if (retry == retry_count)
    {
        // Serial.println("Failed to get time");
        std::cout << "Failed to get time" << std::endl;
    }
    return false;
}

void timeSetting::setTime()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        // Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
    }
    else
    {
        // Serial.println("No time available");
    }
}
