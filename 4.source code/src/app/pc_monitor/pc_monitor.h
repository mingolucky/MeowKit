/**
 * @file pc_monitor.h
 * @author Mingo
 * @brief PC Monitor app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <mooncake.h>
#include "../../MeowKit.h"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief PC Monitor app
     */
    class PCMonitor : public AppAbility {
    public:
        PCMonitor(DEVICES* device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;
    private:
        DEVICES* _device = nullptr;
    };
}
