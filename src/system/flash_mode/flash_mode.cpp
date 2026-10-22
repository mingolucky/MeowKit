/**
 * @file flash_mode.cpp
 * @brief Serial-triggered entry into ROM download mode — implementation.
 */
#include "flash_mode.hpp"

#include <cstring>
#include <esp_system.h>
#include "esp32-hal-tinyusb.h" // usb_persist_restart() / RESTART_BOOTLOADER

namespace {

// Rolling buffer holding the tail of recently received console bytes, so the
// token is matched even when it arrives split across reads.
char   s_buf[48];
size_t s_len = 0;

} // namespace

namespace flash_mode {

void rebootToDownload()
{
    // Reboot into USB download mode. On the ESP32-S3 this must hand the USB PHY
    // from the OTG/TinyUSB controller back to the ROM USB-Serial-JTAG and force
    // a host re-enumerate *before* setting force-download-boot — otherwise the
    // ROM downloader lands on UART0 and never enumerates over USB. Arduino's
    // usb_persist_restart(RESTART_BOOTLOADER) does exactly that (it's the same
    // path as the USB-CDC "1200 bps touch" auto-reset) and then resets.
    //
    // Known quirk: on some hosts this can stall when the board is plugged
    // straight into a root port; it is reliable through a USB hub. It does not
    // return — esp_restart() below is only a safety net if handler registration
    // failed. A normal power cycle always recovers to the app.
    usb_persist_restart(RESTART_BOOTLOADER);
    esp_restart();
    while (true) { /* unreachable */ }
}

void poll(Stream& io)
{
    const size_t token_len = strlen(kTriggerToken);

    while (io.available() > 0) {
        const char c = static_cast<char>(io.read());

        if (s_len >= sizeof(s_buf) - 1) {
            // Drop the oldest byte to make room (keep the most recent tail).
            memmove(s_buf, s_buf + 1, sizeof(s_buf) - 2);
            s_len = sizeof(s_buf) - 2;
        }
        s_buf[s_len++] = c;
        s_buf[s_len]   = '\0';

        if (s_len >= token_len &&
            strcmp(s_buf + (s_len - token_len), kTriggerToken) == 0) {
            io.printf("[FLASH] token received — rebooting into download mode\n");
            io.flush();
            delay(50);
            rebootToDownload();
        }
    }
}

} // namespace flash_mode
