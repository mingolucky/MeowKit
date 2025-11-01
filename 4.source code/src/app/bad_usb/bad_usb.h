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
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <vector>

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief BadUSB app
     */
    class BadUSB : public AppAbility
    {
    public:
        BadUSB(DEVICES *device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;

    private:
        // UI drawing
        void drawHeader();
        void drawSelection();
        void drawStatus(const char *status);
        void redrawAll(const char *status);

        // SD & script
        void scanScripts();
        bool loadCurrentScript();

        // Execution Engine
        void resetExec();
        void stepExecution();

        // Parse/execute a line
        void execCommand(const String &line);
        bool matchToken(const String &s, const char *token);
        uint8_t keyFromName(const String &name, bool &isPrintable, uint8_t &printableChar);

    private:
        DEVICES *_device = nullptr;
        USBHIDKeyboard Keyboard;

        // Running status
        bool _sdReady = false;
        bool _running = false;
        bool _paused = false;
        uint32_t _nextTime = 0; // Non-blocking delay control

        // Script Management
        std::vector<String> _scripts; // /badusb/*.txt List (full path)
        int _sel = 0;                 // The currently selected script index
        std::vector<String> _lines;   // Loaded script lines
        size_t _ip = 0;               // Instruction pointer (current line)
    };
}
