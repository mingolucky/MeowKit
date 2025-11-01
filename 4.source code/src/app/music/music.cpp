/**
 * @file music.cpp
 * @author Mingo
 * @brief MUSIC app implementation for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */

#include "music.h"
// cover image asset for MUSIC app
extern const lv_img_dsc_t ui_img_cover_png;
// include SquareLine assets to access button images (A/B)
#include "assets/ui.h"
extern const lv_img_dsc_t ui_img_ui_normal_btn_a_png;
extern const lv_img_dsc_t ui_img_ui_normal_btn_b_png;

namespace MOONCAKE::APPS
{
    MUSIC::MUSIC(DEVICES *device)
        : _device(device)
    {
        setAppInfo().name = "MUSIC";
    }

    void MUSIC::onOpen()
    {
        // Check if SD card is already initialized
        if (!_device->sd.isInited())
        {
            // Try to initialize if not already done
            if (!_device->sd.init())
            {
                return;
            }
        }

        // Initialize ES8311
        if (!_device->es8311.begin(HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL, 400000))
        {
            return;
        }

        _device->es8311.setSampleRate(44100);
        _device->es8311.setBitsPerSample(16);
        _device->es8311.setVolume(70);
        delay(100);

        // Enable PA amplifier
        _device->io.pinMode(PA_EN, OUTPUT);
        _device->io.digitalWrite(PA_EN, HIGH);
        delay(200);

        // Initialize I2S audio
        _audio.setPinout(14, 13, 45, 38); // BCLK, WS, DOUT, MCLK
        _audio.setVolume(20);

        // Get MP3 file list from /music directory
        _musicFiles.clear();
        File musicDir = SD_MMC.open("/music");
        if (musicDir && musicDir.isDirectory())
        {
            File file = musicDir.openNextFile();
            while (file)
            {
                if (!file.isDirectory())
                {
                    String fileName = file.name();
                    if (fileName.endsWith(".mp3") || fileName.endsWith(".MP3"))
                    {
                        String fullPath = "/music/" + fileName;
                        _musicFiles.push_back(fullPath);
                    }
                }
                file.close();
                file = musicDir.openNextFile();
            }
            musicDir.close();
        }

        if (_musicFiles.empty())
        {
            _device->io.digitalWrite(PA_EN, LOW);
            return;
        }

        // Initialize the playback state (do not start automatically, wait for A to be clicked)
        _currentIndex = 0;
        _isPlaying = false;
        _userStarted = false;
        _initialized = true;

        // ----- Draw the background and bottom button directly (without LVGL refresh)-----
        _device->Lcd.fillScreen(TFT_BLACK);
        // Fix color deviation: Enable byte swapping to match RGB565 data endianness
        _device->Lcd.setSwapBytes(true);
        // Draw the cover 30px down and 10px to the right
        {
            int screenW = _device->Lcd.width();
            int screenH = _device->Lcd.height();
            int imgW = ui_img_cover_png.header.w;
            int imgH = ui_img_cover_png.header.h;
            int shiftY = -45; // Shift pixels down (leave 45 black lines at the top)
            int shiftX = 20;  // Shift right pixels (leave 20 columns black on the left)
            int copyH = min(imgH, screenH - shiftY);
            int copyW = min(imgW, screenW - shiftX);
            if (copyH > 0 && copyW > 0)
            {
                const uint16_t *pdata = (const uint16_t *)ui_img_cover_png.data;
                // Crop line by line and paste to screen with offset (dstX=shiftX, dstY=shiftY)
                for (int y = 0; y < copyH; ++y)
                {
                    const uint16_t *srcRow = pdata + y * imgW; // Do not crop the source, pan down/right as a whole
                    _device->Lcd.pushImage(shiftX, shiftY + y, copyW, 1, srcRow);
                }
                // Fill the right/bottom empty space
                if (screenW > shiftX + copyW)
                {
                    _device->Lcd.fillRect(shiftX + copyW, shiftY, screenW - (shiftX + copyW), copyH, TFT_BLACK);
                }
                if (screenH > shiftY + copyH)
                {
                    _device->Lcd.fillRect(0, shiftY + copyH, screenW, screenH - (shiftY + copyH), TFT_BLACK);
                }
                if (shiftX > 0)
                {
                    _device->Lcd.fillRect(0, shiftY, shiftX, copyH, TFT_BLACK);
                }
                if (shiftY > 0)
                {
                    _device->Lcd.fillRect(0, 0, screenW, shiftY, TFT_BLACK);
                }
            }
            else
            {
                // Fallback: no clipping, direct drawing
                _device->Lcd.pushImage(0, 0, imgW, imgH, (const uint16_t *)ui_img_cover_png.data);
            }
        }

        // Calculate the A/B button position relative to the center and draw the button and the text on the right
        {
            int cx = _device->Lcd.width() / 2;
            int cy = _device->Lcd.height() / 2;

            int aW = ui_img_ui_normal_btn_a_png.header.w;
            int aH = ui_img_ui_normal_btn_a_png.header.h;
            int bW = ui_img_ui_normal_btn_b_png.header.w;
            int bH = ui_img_ui_normal_btn_b_png.header.h;

            // A: (-45, 100) Move left again 20 + 10 = 30
            int a_center_x = (cx - 45) - 30;
            int a_center_y = cy + 100;
            int a_x = a_center_x - aW / 2;
            int a_y = a_center_y - aH / 2;
            // The button material is LV_IMG_CF_TRUE_COLOR_ALPHA, which needs to be alpha pre-mixed with the background before being pushed
            {
                // Skip by pixel transparency and push by continuous segments to avoid covering the background
                static uint16_t lineBuf[64]; // Each segment shall not exceed the button width
                const uint8_t *src = (const uint8_t *)ui_img_ui_normal_btn_a_png.data;
                int w = aW, h = aH;
                for (int y = 0; y < h; ++y)
                {
                    const uint8_t *row = src + (y * w * 3); // Per pixel: 2 bytes RGB565(LE) + 1 byte Alpha
                    int runStart = -1;
                    int runLen = 0;
                    for (int x = 0; x < w; ++x)
                    {
                        uint16_t c565 = (uint16_t)(row[x * 3 + 0] | (row[x * 3 + 1] << 8));
                        uint8_t a = row[x * 3 + 2];
                        if (a == 0)
                        {
                            if (runLen > 0)
                            {
                                // flush current run
                                _device->Lcd.pushImage(a_x + runStart, a_y + y, runLen, 1, lineBuf);
                                runStart = -1;
                                runLen = 0;
                            }
                            continue;
                        }
                        // Non-transparent pixels: ignore translucency and draw the original color directly (best balance between speed and effect)
                        if (runLen == 0)
                            runStart = x;
                        lineBuf[runLen++] = c565;
                        if (runLen == (int)(sizeof(lineBuf) / sizeof(lineBuf[0])))
                        {
                            _device->Lcd.pushImage(a_x + runStart, a_y + y, runLen, 1, lineBuf);
                            runStart = -1;
                            runLen = 0;
                        }
                    }
                    if (runLen > 0)
                    {
                        _device->Lcd.pushImage(a_x + runStart, a_y + y, runLen, 1, lineBuf);
                    }
                }
            }

            // B: (70, 100) shifted left by 10
            int b_center_x = cx + 70 - 10;
            int b_center_y = cy + 100;
            int b_x = b_center_x - bW / 2;
            int b_y = b_center_y - bH / 2;
            {
                static uint16_t lineBuf[64];
                const uint8_t *src = (const uint8_t *)ui_img_ui_normal_btn_b_png.data;
                int w = bW, h = bH;
                for (int y = 0; y < h; ++y)
                {
                    const uint8_t *row = src + (y * w * 3);
                    int runStart = -1;
                    int runLen = 0;
                    for (int x = 0; x < w; ++x)
                    {
                        uint16_t c565 = (uint16_t)(row[x * 3 + 0] | (row[x * 3 + 1] << 8));
                        uint8_t a = row[x * 3 + 2];
                        if (a == 0)
                        {
                            if (runLen > 0)
                            {
                                _device->Lcd.pushImage(b_x + runStart, b_y + y, runLen, 1, lineBuf);
                                runStart = -1;
                                runLen = 0;
                            }
                            continue;
                        }
                        if (runLen == 0)
                            runStart = x;
                        lineBuf[runLen++] = c565;
                        if (runLen == (int)(sizeof(lineBuf) / sizeof(lineBuf[0])))
                        {
                            _device->Lcd.pushImage(b_x + runStart, b_y + y, runLen, 1, lineBuf);
                            runStart = -1;
                            runLen = 0;
                        }
                    }
                    if (runLen > 0)
                    {
                        _device->Lcd.pushImage(b_x + runStart, b_y + y, runLen, 1, lineBuf);
                    }
                }
            }

            // Draw text on the right side of the button
            _device->Lcd.setTextColor(TFT_WHITE);
            _device->Lcd.setFont(&fonts::efontCN_16);
            _device->Lcd.setCursor(a_x + aW + 6, a_center_y - 8);
            _device->Lcd.print("Play/Pause");
            _device->Lcd.setCursor(b_x + bW + 6, b_center_y - 8);
            _device->Lcd.print("Prev/Next");
        }
        // Drawing is finished, restore to default without swapping
        _device->Lcd.setSwapBytes(false);
    }

    void MUSIC::onRunning()
    {
        if (!_initialized || _musicFiles.empty())
        {
            return;
        }

        // Key events: A single click to play; A double click to pause; B single click to play the next song; B double click to play the previous song
        _device->button.A.tick();
        _device->button.B.tick();
        if (_device->button.A.isSingleClick())
        {
            // Play (Wait after entering, start with the first A click; resume playback if paused)
            if (!_userStarted)
            {
                if (startCurrent())
                {
                    _userStarted = true;
                    _paused = false;
                }
            }
            else if (_paused)
            {
                _audio.pauseResume();
                _paused = false;
                _isPlaying = true;
            }
        }
        if (_device->button.A.isDoubleClick())
        {
            // pause
            if (_userStarted && _isPlaying)
            {
                _audio.pauseResume();
                _paused = true;
                _isPlaying = false;
            }
        }
        if (_device->button.B.isSingleClick())
        {
            // Next song
            if (_isPlaying)
            {
                _audio.stopSong();
                _isPlaying = false;
            }
            _currentIndex = (_currentIndex + 1) % _musicFiles.size();
            if (_userStarted)
            {
                startCurrent();
                _paused = false;
            }
        }
        if (_device->button.B.isDoubleClick())
        {
            // Previous song
            if (_isPlaying)
            {
                _audio.stopSong();
                _isPlaying = false;
            }
            int n = (int)_musicFiles.size();
            _currentIndex = (_currentIndex - 1 + n) % n;
            if (_userStarted)
            {
                startCurrent();
                _paused = false;
            }
        }

        // No longer automatically starts playing; only when _userStarted is true and the song is not currently playing, startCurrent() is triggered when the song is switched

        // Keep audio running
        if (_isPlaying)
        {
            _audio.loop();

            // Check if song finished
            if (!_audio.isRunning())
            {
                _isPlaying = false;
                _audio.stopSong();

                // Move to next song
                _currentIndex = (_currentIndex + 1) % _musicFiles.size();
                if (_userStarted)
                {
                    // Play next song continuously
                    startCurrent();
                    _paused = false;
                }
                delay(500); // Brief pause between songs
            }
        }

        // LVGL refresh has been canceled; if you need dynamic text, you can use Lcd.fillRect + Lcd.print to update it yourself
    }

    bool MUSIC::startCurrent()
    {
        if (_musicFiles.empty())
            return false;
        String currentFile = _musicFiles[_currentIndex];
        File testFile = SD_MMC.open(currentFile.c_str());
        if (!testFile)
        {
            _currentIndex = (_currentIndex + 1) % _musicFiles.size();
            return false;
        }
        testFile.close();
        bool connected = _audio.connecttoFS(SD_MMC, currentFile.c_str());
        if (connected)
        {
            _isPlaying = true;
            return true;
        }
        else
        {
            _currentIndex = (_currentIndex + 1) % _musicFiles.size();
            return false;
        }
    }

    void MUSIC::onClose()
    {
        // Stop audio playback
        if (_isPlaying)
        {
            _audio.stopSong();
            _isPlaying = false;
        }

        // Disable PA amplifier
        if (_device)
        {
            _device->io.digitalWrite(PA_EN, LOW);
        }

        // Clear file list
        _musicFiles.clear();
        _currentIndex = 0;
        _initialized = false;

        // Clear screen
        if (_device)
        {
            _device->Lcd.fillScreen(TFT_BLACK);
        }
        // Deleting UI Objects
        if (_infoLabel)
        {
            lv_obj_del(_infoLabel);
            _infoLabel = nullptr;
        }
        if (_btnALabel)
        {
            lv_obj_del(_btnALabel);
            _btnALabel = nullptr;
        }
        if (_btnBLabel)
        {
            lv_obj_del(_btnBLabel);
            _btnBLabel = nullptr;
        }
        if (_btnAImg)
        {
            lv_obj_del(_btnAImg);
            _btnAImg = nullptr;
        }
        if (_btnBImg)
        {
            lv_obj_del(_btnBImg);
            _btnBImg = nullptr;
        }
        if (_bg)
        {
            lv_obj_del(_bg);
            _bg = nullptr;
        }
        if (_screen)
        {
            lv_obj_del(_screen);
            _screen = nullptr;
        }
    }
}
