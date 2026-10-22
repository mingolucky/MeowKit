#include "../include/dino_sfx.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "../../../bsp/devices.h"
#include "../../../system/system_sound.h"
#include "../../../system/settings_bridge.h"
#include <atomic>
#include <cmath>

static Speaker_Class* s_spk   = nullptr;
static TaskHandle_t   s_task  = nullptr;
static QueueHandle_t  s_queue = nullptr;
static SemaphoreHandle_t s_done = nullptr;
static DEVICES* s_device = nullptr;
static std::atomic<uint32_t> s_generation{0};
static std::atomic<bool> s_closing{false};
struct SfxCommand { const uint8_t* score; uint32_t generation; };

/* MIDI note → Hz  (equal temperament, A4 = 440 Hz) */
static uint32_t _midi_hz(uint8_t note)
{
    return (uint32_t)(440.0f * powf(2.0f, ((float)note - 69.0f) / 12.0f) + 0.5f);
}

// Stream PCM directly: tone() adds a silent DMA block after every note,
// which breaks the short 12 ms notes of the jump into audible pulses.
static void _play_note(uint8_t note, uint32_t duration_ms)
{
    const uint32_t rate = s_spk->getSampleRate();
    const uint32_t samples = (uint64_t)rate * duration_ms / 1000;
    const uint32_t ramp = samples / 2 < rate / 500 ? samples / 2 : rate / 500;
    const float omega = 6.28318530718f * _midi_hz(note) / rate;
    int16_t pcm[256];
    for (uint32_t off = 0; off < samples;) {
        const uint32_t count = samples - off > 256 ? 256 : samples - off;
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t pos = off + i;
            const uint32_t edge = pos < samples - 1 - pos ? pos : samples - 1 - pos;
            const float gain = ramp && edge < ramp ? (float)edge / ramp : 1.0f;
            pcm[i] = (int16_t)(8000.0f * gain * sinf(omega * pos));
        }
        if (!s_spk->play(pcm, count)) break;
        off += count;
    }
}

/*
 * Parse and play one ESPboyPlaytune SFX score.
 * Only channel 0 produces audio; other channels contribute delays only.
 * PCM writes block the sound task, not the game.
 */
static void _play_score(const uint8_t* p, uint32_t generation)
{
    while (!s_closing && generation == s_generation.load() &&
           *p != 0xe0 && *p != 0xf0) {
        uint8_t cmd = *p++;
        uint8_t ch  = cmd & 0x0F;

        if ((cmd & 0xF0) == 0x90) {          /* Note ON */
            uint8_t  note     = *p++;
            uint32_t delay_ms = 0;
            if (*p < 0x80) { delay_ms  = (uint32_t)*p++ << 8;
                             delay_ms |= *p++; }
            if (ch == 0 && s_spk && delay_ms > 0)
                _play_note(note, delay_ms);
            else if (delay_ms > 0)
                vTaskDelay(pdMS_TO_TICKS(delay_ms));

        } else if ((cmd & 0xF0) == 0x80) {   /* Note OFF */
            uint32_t delay_ms = 0;
            if (*p < 0x80) { delay_ms  = (uint32_t)*p++ << 8;
                             delay_ms |= *p++; }
            if (delay_ms > 0) vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
        /* 0xex / 0xfx already handled by the while condition */
    }
}

static void _sfx_task(void*)
{
    SfxCommand command{};
    const int16_t silence[256] = {};
    while (true) {
        if (xQueueReceive(s_queue, &command, 0) == pdTRUE && command.score &&
            !s_closing && command.generation == s_generation.load() && sys_get_volume() > 0) {
            _play_score(command.score, command.generation);
        }
        if (s_closing) break;
        // Keep I2S fed with digital silence between effects. Never toggle the
        // codec mute or PA for a short effect: their settling can swallow it.
        if (!s_spk->play(silence, 256)) vTaskDelay(pdMS_TO_TICKS(5));
    }
    size_t remaining = (s_spk->config().dma_buf_count + 1) *
                       s_spk->config().dma_buf_len;
    while (remaining) {
        const size_t count = remaining > 256 ? 256 : remaining;
        if (!s_spk->play(silence, count)) break;
        remaining -= count;
    }
    xSemaphoreGive(s_done);
    vTaskSuspend(nullptr);
}

/* ── Public API ─────────────────────────────────────────────── */

void dino_sfx_init(DEVICES* device)
{
    if (!device || s_device) return;
    system_sound_suspend();
    s_device = device;
    s_spk = &device->speaker;
    s_closing = false;
    if (s_spk->isEnabled() || !s_spk->begin(&In_I2C)) {
        s_spk = nullptr;
        s_device = nullptr;
        system_sound_resume();
        return;
    }
    s_spk->setVolume(sys_get_volume() * SPK_VOLUME_MAX / 100);
    s_spk->setMute(true);
    device->io_exp.digitalWrite(HAL_IOEXP_PA_EN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(12));
    s_spk->setMute(false);

    /* Queue depth 1 — xQueueOverwrite ensures latest request wins */
    s_queue = xQueueCreate(1, sizeof(SfxCommand));
    s_done = xSemaphoreCreateBinary();
    if (!s_queue || !s_done ||
        xTaskCreatePinnedToCore(_sfx_task, "dino_sfx", 4096, nullptr, 3,
                                &s_task, 1) != pdPASS)
        dino_sfx_deinit();
}

void dino_sfx_deinit()
{
    if (!s_device) return;
    s_closing = true;
    ++s_generation;
    if (s_task) {
        SfxCommand stop{};
        xQueueOverwrite(s_queue, &stop);
        xSemaphoreTake(s_done, portMAX_DELAY);
        vTaskDelete(s_task);
        s_task = nullptr;
    }
    if (s_queue) { vQueueDelete(s_queue); s_queue = nullptr; }
    if (s_done) { vSemaphoreDelete(s_done); s_done = nullptr; }
    s_spk->setMute(true);
    vTaskDelay(pdMS_TO_TICKS(4));
    s_device->io_exp.digitalWrite(HAL_IOEXP_PA_EN, LOW);
    s_spk->end();
    s_spk = nullptr;
    s_device = nullptr;
    system_sound_resume();
}

void dino_sfx_play(const uint8_t* score)
{
    if (!s_queue || !score || !s_spk) return;
    SfxCommand command{score, ++s_generation};
    xQueueOverwrite(s_queue, &command);
}

void dino_sfx_stop()
{
    ++s_generation;
    if (s_queue) xQueueReset(s_queue);
}
