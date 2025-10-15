/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2021 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _ES7210_H
#define _ES7210_H

#include <Arduino.h>
#include <Wire.h>

// ES7210 Register Definitions
#define ES7210_RESET_REG00                 0x00   // Reset control
#define ES7210_CLOCK_OFF_REG01             0x01   // Used to turn off the ADC clock
#define ES7210_MAINCLK_REG02               0x02   // Set ADC clock frequency division
#define ES7210_MASTER_CLK_REG03            0x03   // MCLK source $ SCLK division
#define ES7210_LRCK_DIVH_REG04             0x04   // lrck_divh
#define ES7210_LRCK_DIVL_REG05             0x05   // lrck_divl
#define ES7210_POWER_DOWN_REG06            0x06   // power down
#define ES7210_OSR_REG07                   0x07
#define ES7210_MODE_CONFIG_REG08           0x08   // Set master/slave & channels
#define ES7210_TIME_CONTROL0_REG09         0x09   // Set Chip intial state period
#define ES7210_TIME_CONTROL1_REG0A         0x0A   // Set Power up state period
#define ES7210_SDP_INTERFACE1_REG11        0x11   // Set sample & fmt
#define ES7210_SDP_INTERFACE2_REG12        0x12   // Pins state
#define ES7210_ADC_AUTOMUTE_REG13          0x13   // Set mute
#define ES7210_ADC34_MUTERANGE_REG14       0x14   // Set mute range
#define ES7210_ADC34_HPF2_REG20            0x20   // HPF
#define ES7210_ADC34_HPF1_REG21            0x21
#define ES7210_ADC12_HPF1_REG22            0x22
#define ES7210_ADC12_HPF2_REG23            0x23
#define ES7210_ANALOG_REG40                0x40   // ANALOG Power
#define ES7210_MIC12_BIAS_REG41            0x41
#define ES7210_MIC34_BIAS_REG42            0x42
#define ES7210_MIC1_GAIN_REG43             0x43
#define ES7210_MIC2_GAIN_REG44             0x44
#define ES7210_MIC3_GAIN_REG45             0x45
#define ES7210_MIC4_GAIN_REG46             0x46
#define ES7210_MIC1_POWER_REG47            0x47
#define ES7210_MIC2_POWER_REG48            0x48
#define ES7210_MIC3_POWER_REG49            0x49
#define ES7210_MIC4_POWER_REG4A            0x4A
#define ES7210_MIC12_POWER_REG4B           0x4B   // MICBias & ADC & PGA Power
#define ES7210_MIC34_POWER_REG4C           0x4C

#ifdef ES7210_ADDR
    #undef ES7210_ADDR
#endif
// ES7210 I2C Addresses (7-bit)
#define ES7210_ADDR   0x41  // AD1=0, AD0=1  

// Sample rates
enum es7210_sample_rate_t {
    ES7210_8K_SAMPLES = 8000,
    ES7210_11K_SAMPLES = 11025,
    ES7210_16K_SAMPLES = 16000,
    ES7210_22K_SAMPLES = 22050,
    ES7210_24K_SAMPLES = 24000,
    ES7210_32K_SAMPLES = 32000,
    ES7210_44K_SAMPLES = 44100,
    ES7210_48K_SAMPLES = 48000,
    ES7210_96K_SAMPLES = 96000
};

// Bit lengths
enum es7210_bits_t {
    ES7210_16_BITS = 16,
    ES7210_24_BITS = 24,
    ES7210_32_BITS = 32
};

// Audio formats
enum es7210_format_t {
    ES7210_I2S_NORMAL = 0,
    ES7210_I2S_LEFT = 1,
    ES7210_I2S_RIGHT = 2,
    ES7210_I2S_DSP = 3
};

// Microphone input selection
enum es7210_input_mics_t {
    ES7210_INPUT_MIC1 = 0x01,
    ES7210_INPUT_MIC2 = 0x02,
    ES7210_INPUT_MIC3 = 0x04,
    ES7210_INPUT_MIC4 = 0x08
};

// Gain values
enum es7210_gain_value_t {
    GAIN_0DB = 0,
    GAIN_3DB,
    GAIN_6DB,
    GAIN_9DB,
    GAIN_12DB,
    GAIN_15DB,
    GAIN_18DB,
    GAIN_21DB,
    GAIN_24DB,
    GAIN_27DB,
    GAIN_30DB,
    GAIN_33DB,
    GAIN_34_5DB,
    GAIN_36DB,
    GAIN_37_5DB,
};

// Mode definitions
enum es7210_mode_t {
    ES7210_MODE_SLAVE = 0,
    ES7210_MODE_MASTER = 1
};

/**
 * @brief ES7210 Audio ADC Class
 * 
 * This class provides a unified interface for managing the ES7210 4-channel audio ADC.
 * It handles I2C communication, configuration, and control of the ES7210 chip.
 */
class ES7210 {
private:
    // Clock coefficient structure
    struct CoeffDiv {
        uint32_t mclk;            // mclk frequency
        uint32_t lrck;            // lrck
        uint8_t  adc_div;         // adcclk divider
        uint8_t  dll;             // dll_bypass
        uint8_t  doubler;         // doubler enable
        uint8_t  osr;             // adc osr
        uint32_t lrck_h;          // The high 4 bits of lrck
        uint32_t lrck_l;          // The low 8 bits of lrck
    };

    static const CoeffDiv coeffDiv[];
    static const size_t coeffDivSize;
    static const uint32_t MCLK_DIV_FRE = 256;
    static const bool I2S_DSP_MODE = false;

    TwoWire* _wire;
    uint8_t _i2cAddr;
    bool _initialized;
    bool _started;
    
    // Current configuration
    es7210_sample_rate_t _sampleRate;
    es7210_bits_t _bits;
    es7210_format_t _format;
    es7210_mode_t _mode;
    es7210_input_mics_t _micSelect;
    es7210_gain_value_t _gain;
    uint8_t _clockRegValue;

    // Private methods
    esp_err_t writeRegister(uint8_t regAddr, uint8_t data);
    int readRegister(uint8_t regAddr);
    esp_err_t updateRegisterBit(uint8_t regAddr, uint8_t updateBits, uint8_t data);
    int getCoeff(uint32_t mclk, uint32_t lrck);
    esp_err_t resetChip();
    esp_err_t powerUp();
    esp_err_t powerDown();

public:
    /**
     * @brief Constructor
     * 
     * @param wire I2C interface pointer (default: &Wire)
     * @param i2cAddr I2C address (default: ES7210_ADDR)
     */
    ES7210(TwoWire* wire = &Wire, uint8_t i2cAddr = ES7210_ADDR);

    /**
     * @brief Destructor
     */
    ~ES7210();

    /**
     * @brief Initialize the ES7210 with specified configuration
     * 
     * @param sampleRate Sample rate
     * @param bits Bit depth
     * @param format Audio format
     * @param mode Master or slave mode
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t begin(es7210_sample_rate_t sampleRate = ES7210_44K_SAMPLES,
                    es7210_bits_t bits = ES7210_16_BITS,
                    es7210_format_t format = ES7210_I2S_NORMAL,
                    es7210_mode_t mode = ES7210_MODE_SLAVE);

    /**
     * @brief Deinitialize the ES7210
     * 
     * @return esp_err_t ESP_OK on success
     */
    esp_err_t end();

    /**
     * @brief Start the ADC
     * 
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t start();

    /**
     * @brief Stop the ADC
     * 
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t stop();

    /**
     * @brief Configure sample rate
     * 
     * @param sampleRate Sample rate
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t setSampleRate(es7210_sample_rate_t sampleRate);

    /**
     * @brief Configure bit depth
     * 
     * @param bits Bit depth
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t setBits(es7210_bits_t bits);

    /**
     * @brief Configure audio format
     * 
     * @param format Audio format
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t setFormat(es7210_format_t format);

    /**
     * @brief Select microphone inputs
     * 
     * @param mic Microphone selection mask
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t selectMic(es7210_input_mics_t mic);

    /**
     * @brief Set gain for enabled microphones
     * 
     * @param gain Gain value
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t setGain(es7210_gain_value_t gain);

    /**
     * @brief Get current gain value
     * 
     * @return int Gain value (0-14), -1 on error
     */
    int getGain();

    /**
     * @brief Set mute status
     * 
     * @param enable true to mute, false to unmute
     * @return esp_err_t ESP_OK on success
     */
    esp_err_t setMute(bool enable);

    /**
     * @brief Check if device is present
     * 
     * @return true Device is present
     * @return false Device not found
     */
    bool isPresent();

    /**
     * @brief Get initialization status
     * 
     * @return true Initialized
     * @return false Not initialized
     */
    bool isInitialized() const { return _initialized; }

    /**
     * @brief Get running status
     * 
     * @return true Started
     * @return false Stopped
     */
    bool isStarted() const { return _started; }

    /**
     * @brief Get current sample rate
     * 
     * @return es7210_sample_rate_t Current sample rate
     */
    es7210_sample_rate_t getSampleRate() const { return _sampleRate; }

    /**
     * @brief Get current bit depth
     * 
     * @return es7210_bits_t Current bit depth
     */
    es7210_bits_t getBits() const { return _bits; }

    /**
     * @brief Get current format
     * 
     * @return es7210_format_t Current format
     */
    es7210_format_t getFormat() const { return _format; }

    /**
     * @brief Get current mode
     * 
     * @return es7210_mode_t Current mode
     */
    es7210_mode_t getMode() const { return _mode; }

    /**
     * @brief Get selected microphones
     * 
     * @return es7210_input_mics_t Selected microphones
     */
    es7210_input_mics_t getSelectedMic() const { return _micSelect; }

    /**
     * @brief Read register value
     * 
     * @param regAddr Register address
     * @return int Register value, -1 on error
     */
    int readReg(uint8_t regAddr);

    /**
     * @brief Write register value
     * 
     * @param regAddr Register address
     * @param data Data to write
     * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t writeReg(uint8_t regAddr, uint8_t data);

    /**
     * @brief Read all registers for debugging
     */
    void dumpRegisters();

    /**
     * @brief Print current status
     */
    void printStatus();

    /**
     * @brief Get I2C address
     * 
     * @return uint8_t I2C address
     */
    uint8_t getI2CAddress() const { return _i2cAddr; }
};

#endif /* _ES7210_H */