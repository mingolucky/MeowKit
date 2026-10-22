/**
 * @file app_03.cpp
 * @brief App03 — Classic Analog VU Meter
 *
 * Render pipeline:
 *   1. memcpy bgPatch → workSpr   (fresh BG stamp each frame)
 *   2. needle → workSpr           (chroma-keyed rotate/zoom)
 *   3. alpha foreground mask -> workSpr (curved edge only)
 *   4. workSpr -> LCD (single transfer, no ghost trails)
 */
#include "vu_meter.h"
#include "vu_meter_assets.h"
#include "vu_meter_compositor.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include "../../system/system_sound.h"

namespace MOONCAKE::APPS
{

/* ══════════════════════════════════════════════════════════════════
 *  Constants
 * ══════════════════════════════════════════════════════════════════ */

/* ── Angle calibration ─────────────────────────────────────────────
 * ANGLE_MIN → far-left mark  ("−30 VU")
 * ANGLE_MAX → far-right mark ("+3  VU")
 * Angles in degrees, clockwise from straight-up (0°). */
static constexpr float ANGLE_MIN = -45.0f;
static constexpr float ANGLE_MAX =  35.0f;

/* ── Needle sprite geometry ─────────────────────────────────────── */
static constexpr int NEEDLE_W     = 12;
static constexpr int NEEDLE_THICK =  4;
static constexpr int NEEDLE_R     =  2;
static constexpr int NEEDLE_TIP_Y =  2;

static constexpr uint16_t CHROMA16 = TFT_MAGENTA;

/* ── DSP parameters ─────────────────────────────────────────────── */
static constexpr float    kGain        = 4.0f;
static constexpr float    kDbFloor     = -36.0f;
static constexpr float    kDbCeil      =  -6.0f;
static constexpr float    kEmaAttack   = 0.35f;
static constexpr float    kEmaRelease  = 0.08f;
static constexpr uint32_t kSampleRate  = 16000;
static constexpr size_t   kDmaBufSamp  = 256;
static constexpr int      kDmaBufCount = 6;

static constexpr uint32_t kFrameMs     = 33;
static constexpr uint32_t kHeartbeatMs = 500;
static constexpr float    kAngleThr    = 0.3f;

/* ══════════════════════════════════════════════════════════════════
 *  Constructor
 * ══════════════════════════════════════════════════════════════════ */
App03::App03(DEVICES* device) : _device(device)
{
    setAppInfo().name = "VU Meter";
}

/* ══════════════════════════════════════════════════════════════════
 *  Static helpers
 * ══════════════════════════════════════════════════════════════════ */

float App03::_calcAmplitude(const int16_t* buf, size_t mono)
{
    if (!mono) return kDbFloor;
    double sum = 0.0;
    for (size_t i = 0; i < mono; ++i) {
        double s = (double)buf[i] * kGain;
        sum += s * s;
    }
    float rms = (float)sqrt(sum / (double)mono);
    if (rms < 1.0f) return kDbFloor;
    return 20.0f * log10f(rms / 32767.0f);
}

/* ══════════════════════════════════════════════════════════════════
 *  onOpen — returns immediately, no visual change
 *  Previous screen stays visible until BG PNG is pushed in _doHeavyInit
 * ══════════════════════════════════════════════════════════════════ */
void App03::onOpen()
{
    /* ES7210 RX and ES8311 TX share MCLK/BCLK/WS pins. Give app03 exclusive
     * ownership before I2S1 changes the GPIO matrix. */
    system_sound_suspend();

    // If a previous crash left the I2S driver installed, uninstall it now.
    // esp_err_t is ignored — the call is a no-op when the driver is absent.
    i2s_driver_uninstall(I2S_NUM_1);

    _initState        = InitState::Loading;
    _taskRun          = false;
    _captureTask      = nullptr;
    _latestAngle.store(ANGLE_MIN);
    _lastFrameMs      = 0;
    _lastRenderAngle  = -999.0f;
    _pivotX           = kPivotX;
    _pivotY           = kPivotY;
}

/* ══════════════════════════════════════════════════════════════════
 *  _doHeavyInit
 * ══════════════════════════════════════════════════════════════════ */
void App03::_doHeavyInit()
{
    auto fail = [&](const char* msg) {
        _releaseResources();
        _device->Lcd.fillScreen(TFT_BLACK);
        _device->Lcd.setTextColor(TFT_RED, TFT_BLACK);
        _device->Lcd.setTextFont(2);
        _device->Lcd.drawCentreString(msg, 160, 110);
        _initState = InitState::Failed;
    };

    if (!vu_meter_bg_png_len || !vu_meter_mask_png_len) {
        fail("VU assets missing"); return;
    }

    /* 1. ES7210 codec */
    if (!_codec.begin(kSampleRate, ES7210_BIT_16, ES7210_FMT_I2S)) {
        fail("ES7210 init failed"); return;
    }
    _codecReady = true;
    _codec.selectMic(ES7210_MIC1 | ES7210_MIC2);
    _codec.setGain(ES7210_GAIN_30DB);
    _codec.start();

    /* 2. I2S_NUM_1 RX */
    {
        i2s_config_t cfg = {};
        cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
        cfg.sample_rate          = kSampleRate;
        cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
        cfg.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT;
        cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
        cfg.dma_buf_count        = kDmaBufCount;
        cfg.dma_buf_len          = (int)kDmaBufSamp;
        cfg.use_apll             = true;
        cfg.tx_desc_auto_clear   = false;
        cfg.fixed_mclk           = 0;
        if (i2s_driver_install(I2S_NUM_1, &cfg, 0, nullptr) != ESP_OK) {
            fail("I2S install failed"); return;
        }
        _i2sReady = true;
        i2s_pin_config_t pins = {};
        pins.mck_io_num   = HAL_PIN_I2S_MCLK;
        pins.bck_io_num   = HAL_PIN_I2S_BCLK;
        pins.ws_io_num    = HAL_PIN_I2S_WS;
        pins.data_out_num = I2S_PIN_NO_CHANGE;
        pins.data_in_num  = HAL_PIN_I2S_DIN;
        if (i2s_set_pin(I2S_NUM_1, &pins) != ESP_OK) {
            fail("I2S pin cfg failed"); return;
        }
        i2s_start(I2S_NUM_1);
    }

    /* 3. bgPatch — full BG reference covering y=kPatchY..240
     *    kPatchH=160 ensures mask seeding always finds valid BG rows */
    _bgPatch = new LGFX_Sprite(&_device->Lcd);
    _bgPatch->setPsram(true);
    _bgPatch->setColorDepth(16);
    if (!_bgPatch->createSprite(kPatchW, kPatchH)) {
        fail("PSRAM alloc failed (bgPatch)"); return;
    }
    _bgPatch->fillSprite(TFT_BLACK);

    /* 4. workSpr — per-frame compositing buffer (full needle sweep) */
    _workSpr = new LGFX_Sprite(&_device->Lcd);
    _workSpr->setPsram(true);
    _workSpr->setColorDepth(16);
    if (!_workSpr->createSprite(kPatchW, kWorkH)) {
        fail("PSRAM alloc failed (workSpr)"); return;
    }

    /* 5. Cache foreground color + alpha once. Transparent mask pixels must
     * leave the needle visible; flattening onto BG makes an opaque rectangle. */
    _maskPixels = static_cast<uint32_t*>(heap_caps_malloc(
        vu_meter::maskW * vu_meter::maskH * sizeof(uint32_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!_maskPixels || !vu_meter::decodeMask(
            vu_meter_mask_png, vu_meter_mask_png_len, _maskPixels)) {
        fail("VU mask decode failed"); return;
    }
    if (!_device->Lcd.drawPng(vu_meter_bg_png, vu_meter_bg_png_len, 0, 0)
        || !_bgPatch->drawPng(vu_meter_bg_png, vu_meter_bg_png_len,
                              -kPatchX, -kPatchY)) {
        fail("VU background decode failed"); return;
    }

    /* 6. Needle sprite — pivot row = NEEDLE_TIP_Y + kNeedleLen */
    {
        const int pivY = NEEDLE_TIP_Y + kNeedleLen;
        const int sprH = pivY + 5;
        LGFX_Sprite* ns = new LGFX_Sprite(&_device->Lcd);
        ns->setPsram(true);
        ns->setColorDepth(16);
        if (ns->createSprite(NEEDLE_W, sprH)) {
            const int cx = NEEDLE_W / 2;
            ns->fillSprite(CHROMA16);
            /* Shadow: dark grey, 1 px right + 1 px down */
            ns->fillRoundRect(cx - NEEDLE_THICK/2 + 1, NEEDLE_TIP_Y + 1,
                              NEEDLE_THICK, kNeedleLen - 2, NEEDLE_R, 0x2104u);
            /* Body: white */
            ns->fillRoundRect(cx - NEEDLE_THICK/2, NEEDLE_TIP_Y,
                              NEEDLE_THICK, kNeedleLen - 2, NEEDLE_R, TFT_WHITE);
            ns->setPivot((float)cx, (float)pivY);
            _needleSprite = ns;
        } else {
            delete ns;
            fail("PSRAM alloc failed (needle)"); return;
        }
    }

    /* 7. Start DSP capture task on core 0 */
    _taskRun = true;
    if (xTaskCreatePinnedToCore(_captureTaskEntry, "vu_cap",
                                4096, this, 2, &_captureTask, 0) != pdPASS) {
        _taskRun = false;
        fail("Capture task failed"); return;
    }

    _initState = InitState::Ready;
}

/* ══════════════════════════════════════════════════════════════════
 *  onClose
 * ══════════════════════════════════════════════════════════════════ */
void App03::_releaseResources()
{
    _taskRun = false;
    if (_captureTask) {
        for (int i = 0; i < 25 && _captureTask; ++i)
            vTaskDelay(pdMS_TO_TICKS(10));
        if (_captureTask) {
            vTaskDelete(_captureTask);
            _captureTask = nullptr;
        }
    }

    if (_codecReady) {
        _codec.stop();
        _codec.end();
        _codecReady = false;
    }
    if (_i2sReady) {
        i2s_stop(I2S_NUM_1);
        i2s_driver_uninstall(I2S_NUM_1);
        _i2sReady = false;
    }

    auto del = [](LGFX_Sprite*& sp) {
        if (sp) { sp->deleteSprite(); delete sp; sp = nullptr; }
    };
    del(_bgPatch);
    del(_workSpr);
    del(_needleSprite);
    if (_maskPixels) { heap_caps_free(_maskPixels); _maskPixels = nullptr; }
}

void App03::onClose()
{
    _releaseResources();

    _initState = InitState::Idle;
    _device->Lcd.fillScreen(TFT_BLACK);

    /* I2S1 is fully released; restore ES8311/I2S0 and reclaim shared pins. */
    if (!system_sound_resume())
        Serial.println("[VU] Failed to restore system audio");
}

/* ══════════════════════════════════════════════════════════════════
 *  onRunning — ~30 fps
 * ══════════════════════════════════════════════════════════════════ */
void App03::onRunning()
{
    if (_initState == InitState::Loading) {
        _doHeavyInit();
        return;
    }
    if (_initState == InitState::Failed || !_bgPatch || !_workSpr) return;

    float    ang = _latestAngle.load();
    uint32_t now = millis();

    bool angleJump = fabsf(ang - _lastRenderAngle) >= kAngleThr;
    bool heartbeat = (now - _lastFrameMs) >= kHeartbeatMs;
    bool frameDue  = (now - _lastFrameMs) >= kFrameMs;

    if (!angleJump && !heartbeat) return;
    if (!frameDue)                return;

    _lastFrameMs     = now;
    _lastRenderAngle = ang;

    /* 1. Stamp fresh BG into work sprite (memcpy ~kPatchW×kWorkH×2 bytes) */
    memcpy(_workSpr->getBuffer(),
           _bgPatch->getBuffer(),
           (size_t)kPatchW * kWorkH * sizeof(uint16_t));

    /* 2. Draw the whole needle into the background patch. */
    if (_needleSprite)
        _needleSprite->pushRotateZoom(_workSpr,
                                      _pivotX - (float)kPatchX,
                                      _pivotY - (float)kPatchY,
                                      ang, 1.0f, 1.0f, kChroma24);

    /* 3. Apply only the foreground's nontransparent pixels. Cache decoding
     * at startup and blend in-place to retain the curved antialiased edge. */
    if (_maskPixels)
        vu_meter::compositeMask(static_cast<uint16_t*>(_workSpr->getBuffer()),
                               kPatchW, kWorkH,
                               vu_meter::maskX - kPatchX,
                               vu_meter::maskY - kPatchY, _maskPixels);

    /* 4. Present the fully composed frame in one LCD transaction. */
    _workSpr->pushSprite(&_device->Lcd, kPatchX, kPatchY);
}

/* ══════════════════════════════════════════════════════════════════
 *  DSP capture task — core 0
 * ══════════════════════════════════════════════════════════════════ */
void App03::_captureTaskEntry(void* arg)
{
    auto* self = static_cast<App03*>(arg);
    self->_captureLoop();
    self->_captureTask = nullptr;
    vTaskDelete(nullptr);
}

void App03::_captureLoop()
{
    constexpr size_t kStereoSamp = kDmaBufSamp * 2;
    constexpr size_t kBufBytes   = kStereoSamp * sizeof(int16_t);

    int16_t* buf = (int16_t*)heap_caps_malloc(kBufBytes, MALLOC_CAP_INTERNAL);
    if (!buf) { _taskRun = false; return; }

    float   smooth = ANGLE_MIN;
    int32_t dcAcc  = 0;

    while (_taskRun) {
        size_t bytesRead = 0;
        if (i2s_read(I2S_NUM_1, buf, kBufBytes, &bytesRead, pdMS_TO_TICKS(60))
                != ESP_OK || bytesRead < 4)
            continue;

        size_t mono = bytesRead / (2 * sizeof(int16_t));
        for (size_t i = 0; i < mono; ++i) {
            int32_t s = buf[i * 2];
            dcAcc = dcAcc - (dcAcc >> 8) + s;
            buf[i] = (int16_t)(s - (dcAcc >> 8));
        }

        float db     = _calcAmplitude(buf, mono);
        db           = std::max(kDbFloor, std::min(kDbCeil, db));
        float norm   = (db - kDbFloor) / (kDbCeil - kDbFloor);
        float target = ANGLE_MIN + norm * (ANGLE_MAX - ANGLE_MIN);

        float alpha  = (target > smooth) ? kEmaAttack : kEmaRelease;
        smooth      += alpha * (target - smooth);
        _latestAngle.store(smooth);
    }

    free(buf);
}

} // namespace MOONCAKE::APPS
