#include "deviceStateManager.h"

struct DeviceStatus_t _device_status = {
    .updated = true,
    .autoScreenOff = true,
    .sdCardInserted = false,
    .powerDetection = false,
    .brightness = 127,
    .volume = 50,
    .autoScreenOffTime = 20000};