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
#include <stdbool.h>
#include <stdint.h>

struct DeviceStatus_t
{
    bool updated;
    bool autoScreenOff;
    bool sdCardInserted;
    bool powerDetection;
    uint8_t brightness;
    uint8_t volume;
    uint32_t autoScreenOffTime;
};

extern struct DeviceStatus_t _device_status;