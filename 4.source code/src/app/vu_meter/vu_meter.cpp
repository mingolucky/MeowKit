/**
 * @file vu_meter.cpp
 * @author Mingo
 * @brief VU Meter app implementation for Mooncake
 * @version 0.2
 * @date 2025-08-08
 * @copyright Copyright (c) 2025
 */
#include "vu_meter.h"
#include <cmath>
#include <driver/i2s.h>
#include "../../bsp/devices.h"

namespace {

// 采样参数 - 优化实时性
constexpr int   kSampleRate     = 44100;
constexpr int   kBitsPerSample  = I2S_BITS_PER_SAMPLE_16BIT;
constexpr int   kChannels       = 2;                   
constexpr size_t kChunkSamples  = 256;                 // FFT友好的大小
constexpr size_t kDmaBufLen     = 512;                 
constexpr int   kDmaBufCount    = 4;                   

// VU 映射与平滑 - 进一步优化抖动
constexpr float kDbFloor        = -65.0f;              // 扩大底部范围，提高灵敏度
constexpr float kDbCeil         = -25.0f;              // 调整顶部范围，避免过饱和
constexpr float kAttackAlpha    = 0.15f;               // 进一步降低攻击速度，减少突跳
constexpr float kReleaseAlpha   = 0.08f;               // 更平滑的释放
constexpr float kMinRms         = 1.0f;                

// 指针角度范围 - 改为-55°到+55°（110°总范围）
constexpr float kAngleMin       = -55.0f;              // 从-55°开始
constexpr float kAngleMax       = 55.0f;               // 到+55°结束

// 音频预处理参数 - 加强静默检测
constexpr float kNoiseGate      = 30.0f;               // 提高门限值，减少环境噪音敏感度
constexpr float kSilenceThreshold = 50.0f;             // 静默检测阈值
constexpr uint32_t kSilenceTimeMs = 500;               // 静默持续时间，超过此时间锁定到-60°               

// I2S 引脚配置
constexpr i2s_pin_config_t kI2sPins = {
    .mck_io_num   = HAL_PIN_I2S_MCLK,   // 38
    .bck_io_num   = HAL_PIN_I2S_BCLK,   // 14
    .ws_io_num    = HAL_PIN_I2S_WS,     // 13
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = HAL_PIN_I2S_DIN     // 12
};

inline i2s_config_t makeInputI2SConfig() {
    i2s_config_t c{};
    c.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
    c.sample_rate = kSampleRate;
    c.bits_per_sample = static_cast<i2s_bits_per_sample_t>(kBitsPerSample);
    c.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    c.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    c.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    c.dma_buf_count = kDmaBufCount;
    c.dma_buf_len = kDmaBufLen;
    c.use_apll = true;                                  
    c.tx_desc_auto_clear = false;
    c.fixed_mclk = 0;
#if ESP_IDF_VERSION_MAJOR >= 5
    c.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    c.bits_per_chan = I2S_BITS_PER_CHAN_16BIT;
#endif
    return c;
}

// 工具函数
static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    if (in_max == in_min) return out_min;
    float mapped = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    return mapped;
}

static float constrainFloat(float x, float a, float b) {
    return x < a ? a : (x > b ? b : x);
}

// 简化的幅值计算 - 替代FFT，加强静默检测
static float calculateAmplitude(const int16_t* buffer, size_t samples) {
    double sum = 0.0;
    double sumSquared = 0.0;
    size_t validSamples = 0;
    float maxSample = 0.0f;
    
    for (size_t i = 0; i < samples; i++) {
        float sample = static_cast<float>(buffer[i]);
        float absSample = std::abs(sample);
        
        // 记录最大样本值
        if (absSample > maxSample) {
            maxSample = absSample;
        }
        
        if (absSample > kNoiseGate) {
            sum += absSample;
            sumSquared += sample * sample;
            validSamples++;
        }
    }
    
    // 如果最大样本都低于静默阈值，直接返回0
    if (maxSample < kSilenceThreshold) {
        return 0.0f;
    }
    
    if (validSamples == 0) return 0.0f;
    
    // 使用RMS值作为幅值
    float rms = std::sqrt(sumSquared / validSamples);
    return rms;
}

} // namespace

namespace MOONCAKE::APPS
{
    VUMeter::VUMeter(DEVICES* device)
        : _device(device), _silenceStartTime(0), _inSilence(false)
    {
        setAppInfo().name = "VUMeter";
    }

    void VUMeter::setupPointerPivot_() {
        if (!ui_line) {
            return;
        }

        // 确保对象完全加载
        lv_obj_update_layout(ui_line);
        lv_refr_now(lv_disp_get_default());
        delay(50);
        
        // 获取尺寸
        lv_coord_t w = lv_obj_get_width(ui_line);
        lv_coord_t h = lv_obj_get_height(ui_line);
        lv_coord_t x = lv_obj_get_x(ui_line);
        lv_coord_t y = lv_obj_get_y(ui_line);
        
        // 确保对象可见和可操作
        lv_obj_clear_flag(ui_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_line, LV_OBJ_FLAG_CLICKABLE);
        
        // 设置pivot点 - 在指针底部中央
        lv_coord_t pivot_x = w / 2;
        lv_coord_t pivot_y = h - 8;  // 稍微向上调整
        lv_img_set_pivot(ui_line, pivot_x, pivot_y);
        
        // 设置初始角度
        _angleDeg = 0.0f;
        lv_img_set_angle(ui_line, 0);
    }

    void VUMeter::setPointerAngle_(float deg) {
        if (!ui_line) {
            return;
        }
        
        // 约束角度范围 (-60° 到 +60°)
        deg = constrainFloat(deg, kAngleMin, kAngleMax);
        
        // 检查角度变化，避免无效更新，增加阈值减少抖动
        static float lastAngle = -999.0f;
        if (std::abs(deg - lastAngle) < 1.0f) {  // 增加阈值到1度
            return;  // 角度变化太小，跳过更新
        }
        
        // LVGL角度单位为0.1度，正值为顺时针
        int16_t lvgl_angle = static_cast<int16_t>(deg * 10.0f);
        
        // 设置角度
        lv_img_set_angle(ui_line, lvgl_angle);
        
        // 减少刷新操作，避免残影
        lv_obj_invalidate(ui_line);
        
        _angleDeg = deg;
        lastAngle = deg;
    }

    bool VUMeter::initAudio_() {
        if (!_device) {
            return false;
        }

        // 1) 初始化 ES7210 ADC
        esp_err_t err = _device->es7210.begin(ES7210_44K_SAMPLES, ES7210_16_BITS, ES7210_I2S_NORMAL, ES7210_MODE_SLAVE);
        if (err != ESP_OK) {
            return false;
        }
        
        _device->es7210.selectMic(static_cast<es7210_input_mics_t>(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2));
        _device->es7210.setGain(GAIN_15DB);  // 最大增益提高灵敏度
        
        err = _device->es7210.start();
        if (err != ESP_OK) {
            return false;
        }

        // 2) 初始化 I2S
        auto i2sCfg = makeInputI2SConfig();
        err = i2s_driver_install(_i2sPort, &i2sCfg, 0, nullptr);
        if (err != ESP_OK) {
            return false;
        }

        err = i2s_set_pin(_i2sPort, &kI2sPins);
        if (err != ESP_OK) {
            i2s_driver_uninstall(_i2sPort);
            return false;
        }

        err = i2s_set_clk(_i2sPort, kSampleRate, static_cast<i2s_bits_per_sample_t>(kBitsPerSample), I2S_CHANNEL_STEREO);
        if (err != ESP_OK) {
            // Continue even if clock config fails
        }

        i2s_zero_dma_buffer(_i2sPort);
        
        err = i2s_start(_i2sPort);
        if (err != ESP_OK) {
            i2s_driver_uninstall(_i2sPort);
            return false;
        }

        _emaDb = -90.0f;
        _lastSample = 0.0f;
        _audioReady = true;
        
        return true;
    }

    void VUMeter::deinitAudio_() {
        if (_device) {
            _device->es7210.stop();
        }
        
        if (_audioReady) {
            i2s_stop(_i2sPort);
            i2s_driver_uninstall(_i2sPort);
        }
        
        _audioReady = false;
    }

    void VUMeter::onOpen()
    {
        // 加载 UI
        ui_VU_Meter_screen_init();
        lv_scr_load_anim(ui_VU_Meter, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);

        // 等待UI完全加载
        lv_refr_now(lv_disp_get_default());
        delay(300);
        
        if (!ui_line) {
            return;
        }

        // 设置指针
        setupPointerPivot_();

        // 连续摆动测试 - 在-60°到+60°范围内
        for (int cycle = 0; cycle < 1; cycle++) {  // 减少循环次数
            for (int deg = -60; deg <= 60; deg += 5) {  // 增加步长，减少残影
                setPointerAngle_(deg);
                lv_refr_now(lv_disp_get_default());
                delay(50);  // 增加延时
            }
            for (int deg = 60; deg >= -60; deg -= 5) {
                setPointerAngle_(deg);
                lv_refr_now(lv_disp_get_default());
                delay(50);
            }
        }
        
        setPointerAngle_(0.0f);

        // 初始化音频
        if (!initAudio_()) {
            return;
        }
    }

    void VUMeter::updateVu_() {
        if (!_audioReady || !ui_line) return;

        static int16_t buffer[kChunkSamples * kChannels];
        static uint32_t lastRefreshTime = 0;
        size_t bytesRead = 0;

        // 读取音频数据
        esp_err_t err = i2s_read(_i2sPort, buffer, sizeof(buffer), &bytesRead, 1);
        
        if (err != ESP_OK || bytesRead == 0) {
            // 静音时缓慢回到中心位置 (0°)
            _emaDb = _emaDb * 0.98f + (-90.0f) * 0.02f;
            float angle = mapFloat(_emaDb, kDbFloor, kDbCeil, kAngleMin, kAngleMax);
            angle = constrainFloat(angle, kAngleMin, kAngleMax);
            setPointerAngle_(angle);
            // 降低刷新频率，避免残影
            uint32_t currentTime = millis();
            if (currentTime - lastRefreshTime > 50) {  // 20FPS刷新
                lv_refr_now(lv_disp_get_default());
                lastRefreshTime = currentTime;
            }
            return;
        }

        const size_t samples = bytesRead / sizeof(int16_t);
        if (samples < 4) return;

        // 计算音频幅值 (替代FFT)
        float amplitude = calculateAmplitude(buffer, samples);
        
        uint32_t currentTime = millis();
        
        if (amplitude > 0) {
            // 检测到有效音频信号，退出静默状态
            if (_inSilence) {
                _inSilence = false;
            }
            _silenceStartTime = 0;  // 重置静默计时器
            
            // 转换为 dBFS
            double dbfs = 20.0 * std::log10((amplitude + kMinRms) / 32768.0);
            dbfs = std::max(-90.0, std::min(0.0, dbfs));

            // 更温和的平滑处理
            float alpha = (dbfs > _emaDb) ? kAttackAlpha : kReleaseAlpha;
            _emaDb += alpha * (static_cast<float>(dbfs) - _emaDb);

            // 映射到-60°到+60°角度范围
            float angle = mapFloat(_emaDb, kDbFloor, kDbCeil, kAngleMin, kAngleMax);
            angle = constrainFloat(angle, kAngleMin, kAngleMax);
            
            // 更新指针
            setPointerAngle_(angle);
            
            // 降低刷新频率，减少残影
            if (currentTime - lastRefreshTime > 33) {  // 约30FPS刷新率
                lv_refr_now(lv_disp_get_default());
                lastRefreshTime = currentTime;
                
                // 串口打印频率与指针刷新频率一致
                Serial0.printf("[VU] Amplitude: %.0f dB: %.1f\n", amplitude, _emaDb);
            }
        } else {
            // 检测到静默
            if (!_inSilence) {
                if (_silenceStartTime == 0) {
                    _silenceStartTime = currentTime;
                } else if (currentTime - _silenceStartTime > kSilenceTimeMs) {
                    // 进入静默模式
                    _inSilence = true;
                    
                    // 平滑地将指针移动到-60°位置
                    _emaDb = kDbFloor;
                    setPointerAngle_(kAngleMin);
                    lv_refr_now(lv_disp_get_default());
                }
            } else {
                // 已经在静默状态，保持指针在-60°不动
                // 不做任何更新，避免抖动
            }
        }
    }

    void VUMeter::onRunning()
    {
        updateVu_();
        
        // 添加测试模式：如果长按某个按键，进入自动摆动测试
        static uint32_t testModeStart = 0;
        static bool testMode = false;
        
        // 检查是否进入测试模式（这里可以替换为实际的按键检测）
        if (_device && millis() - testModeStart > 10000 && !testMode) {  // 10秒后进入测试
            testMode = true;
        }
        
        // 测试模式：更自然的摆动，模拟真实VU表
        if (testMode) {
            static uint32_t lastTestUpdate = 0;
            static float testAngle = 0.0f;
            static float testDirection = 1.0f;
            
            uint32_t currentTime = millis();
            if (currentTime - lastTestUpdate > 100) {  // 降低到10Hz更新，更自然
                testAngle += testDirection * 1.5f;  // 减小步长
                
                if (testAngle >= 60.0f) {     // 改为+60°范围
                    testAngle = 60.0f;
                    testDirection = -1.0f;
                } else if (testAngle <= -60.0f) {  // 改为-60°范围
                    testAngle = -60.0f;
                    testDirection = 1.0f;
                }
                
                setPointerAngle_(testAngle);
                // 降低测试模式的刷新频率
                if (currentTime % 200 == 0) {  // 每200ms刷新一次
                    lv_refr_now(lv_disp_get_default());
                }
                
                lastTestUpdate = currentTime;
            }
        }
    }

    void VUMeter::onClose()
    {
        deinitAudio_();
        
        if (_device && _device->Lcd.width() > 0 && _device->Lcd.height() > 0) {
            _device->Lcd.fillScreen(TFT_BLACK);
        }
    }
}