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

#include "es7210.h"

#define TAG_ES7210 "ES7210"

// Clock coefficient table
const ES7210::CoeffDiv ES7210::coeffDiv[] = {
    //mclk      lrck    adc_div  dll  doubler osr  lrckh   lrckl
    /* 8k */
    {12288000,  8000 ,  0x03,  0x01,  0x00,  0x20,  0x06,  0x00},
    {16384000,  8000 ,  0x04,  0x01,  0x00,  0x20,  0x08,  0x00},
    {19200000,  8000 ,  0x1e,  0x00,  0x01,  0x28,  0x09,  0x60},
    {4096000,   8000 ,  0x01,  0x01,  0x00,  0x20,  0x02,  0x00},

    /* 11.025k */
    {11289600,  11025,  0x02,  0x01,  0x00,  0x20,  0x01,  0x00},

    /* 12k */
    {12288000,  12000,  0x02,  0x01,  0x00,  0x20,  0x04,  0x00},
    {19200000,  12000,  0x14,  0x00,  0x01,  0x28,  0x06,  0x40},

    /* 16k */
    {4096000,   16000,  0x01,  0x01,  0x01,  0x20,  0x01,  0x00},
    {19200000,  16000,  0x0a,  0x00,  0x00,  0x1e,  0x04,  0x80},
    {16384000,  16000,  0x02,  0x01,  0x00,  0x20,  0x04,  0x00},
    {12288000,  16000,  0x03,  0x01,  0x01,  0x20,  0x03,  0x00},

    /* 22.05k */
    {11289600,  22050,  0x01,  0x01,  0x00,  0x20,  0x02,  0x00},

    /* 24k */
    {12288000,  24000,  0x01,  0x01,  0x00,  0x20,  0x02,  0x00},
    {19200000,  24000,  0x0a,  0x00,  0x01,  0x28,  0x03,  0x20},

    /* 32k */
    {12288000,  32000,  0x03,  0x00,  0x00,  0x20,  0x01,  0x80},
    {16384000,  32000,  0x01,  0x01,  0x00,  0x20,  0x02,  0x00},
    {19200000,  32000,  0x05,  0x00,  0x00,  0x1e,  0x02,  0x58},

    /* 44.1k */
    {11289600,  44100,  0x01,  0x01,  0x01,  0x20,  0x01,  0x00},

    /* 48k */
    {12288000,  48000,  0x01,  0x01,  0x01,  0x20,  0x01,  0x00},
    {19200000,  48000,  0x05,  0x00,  0x01,  0x28,  0x01,  0x90},

    /* 64k */
    {16384000,  64000,  0x01,  0x01,  0x00,  0x20,  0x01,  0x00},
    {19200000,  64000,  0x05,  0x00,  0x01,  0x1e,  0x01,  0x2c},

    /* 88.2k */
    {11289600,  88200,  0x01,  0x01,  0x01,  0x20,  0x00,  0x80},

    /* 96k */
    {12288000,  96000,  0x01,  0x01,  0x01,  0x20,  0x00,  0x80},
    {19200000,  96000,  0x05,  0x00,  0x01,  0x28,  0x00,  0xc8},
};

const size_t ES7210::coeffDivSize = sizeof(ES7210::coeffDiv) / sizeof(ES7210::coeffDiv[0]);

// Constructor
ES7210::ES7210(TwoWire* wire, uint8_t i2cAddr) {
    _wire = wire;
    _i2cAddr = i2cAddr;
    _initialized = false;
    _started = false;
    
    // Default configuration
    _sampleRate = ES7210_44K_SAMPLES;
    _bits = ES7210_16_BITS;
    _format = ES7210_I2S_NORMAL;
    _mode = ES7210_MODE_SLAVE;
    _micSelect = static_cast<es7210_input_mics_t>(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2);
    _gain = GAIN_30DB;
    _clockRegValue = 0x00;
}

// Destructor
ES7210::~ES7210() {
    if (_initialized) {
        end();
    }
}

// Initialize ES7210
esp_err_t ES7210::begin(es7210_sample_rate_t sampleRate, es7210_bits_t bits, 
                        es7210_format_t format, es7210_mode_t mode) {
    
    // Store configuration
    _sampleRate = sampleRate;
    _bits = bits;
    _format = format;
    _mode = mode;
    
    // Check if device is present
    if (!isPresent()) {
        Serial0.printf("[%s] Device not found at address 0x%02X!\n", TAG_ES7210, _i2cAddr);
        return ESP_FAIL;
    }
    
    Serial0.printf("[%s] Device found at address 0x%02X\n", TAG_ES7210, _i2cAddr);
    
    esp_err_t ret = ESP_OK;
    
    // Reset chip
    ret |= resetChip();
    if (ret != ESP_OK) {
        Serial0.printf("[%s] Reset failed\n", TAG_ES7210);
        return ret;
    }
    
    // Basic setup
    ret |= writeRegister(ES7210_CLOCK_OFF_REG01, 0x1f);
    ret |= writeRegister(ES7210_TIME_CONTROL0_REG09, 0x30);      // Set chip state cycle
    ret |= writeRegister(ES7210_TIME_CONTROL1_REG0A, 0x30);      // Set power on state cycle
    ret |= writeRegister(ES7210_ADC12_HPF2_REG23, 0x2a);         // Quick setup
    ret |= writeRegister(ES7210_ADC12_HPF1_REG22, 0x0a);
    ret |= writeRegister(ES7210_ADC34_HPF2_REG20, 0x0a);
    ret |= writeRegister(ES7210_ADC34_HPF1_REG21, 0x2a);
    
    // Set master/slave mode
    switch (_mode) {
        case ES7210_MODE_MASTER:
            Serial0.printf("[%s] ES7210 in Master mode\n", TAG_ES7210);
            ret |= updateRegisterBit(ES7210_MODE_CONFIG_REG08, 0x01, 0x01);
            ret |= updateRegisterBit(ES7210_MASTER_CLK_REG03, 0x80, 0x00);
            break;
        case ES7210_MODE_SLAVE:
            Serial0.printf("[%s] ES7210 in Slave mode\n", TAG_ES7210);
            ret |= updateRegisterBit(ES7210_MODE_CONFIG_REG08, 0x01, 0x00);
            break;
    }
    
    // Analog configuration
    ret |= writeRegister(ES7210_ANALOG_REG40, 0x43);               // Select power off analog, vdda = 3.3V
    ret |= writeRegister(ES7210_MIC12_BIAS_REG41, 0x70);           // Select 2.87v
    ret |= writeRegister(ES7210_MIC34_BIAS_REG42, 0x70);           // Select 2.87v
    ret |= writeRegister(ES7210_OSR_REG07, 0x20);
    ret |= writeRegister(ES7210_MAINCLK_REG02, 0xc1);              // Set frequency division coefficient
    
    // Configure sample rate, bits, and format
    ret |= setSampleRate(_sampleRate);
    ret |= setBits(_bits);
    ret |= setFormat(_format);
    
    // Select microphones and set gain
    ret |= selectMic(_micSelect);
    ret |= setGain(_gain);
    
    if (ret == ESP_OK) {
        _initialized = true;
        Serial0.printf("[%s] Initialized successfully\n", TAG_ES7210);
        printStatus();
    } else {
        Serial0.printf("[%s] Initialization failed\n", TAG_ES7210);
    }
    
    return ret;
}

// Deinitialize ES7210
esp_err_t ES7210::end() {
    if (_started) {
        stop();
    }
    
    powerDown();
    _initialized = false;
    
    Serial0.printf("[%s] Deinitialized\n", TAG_ES7210);
    return ESP_OK;
}

// Start ADC
esp_err_t ES7210::start() {
    if (!_initialized) {
        Serial0.printf("[%s] Device not initialized\n", TAG_ES7210);
        return ESP_FAIL;
    }
    
    esp_err_t ret = ESP_OK;
    
    // Read current clock register value
    int regv = readRegister(ES7210_CLOCK_OFF_REG01);
    if ((regv != 0x7f) && (regv != 0xff) && (regv >= 0)) {
        _clockRegValue = regv;
    }
    
    Serial0.printf("[%s] Starting ADC with clock reg value: 0x%02X\n", TAG_ES7210, _clockRegValue);
    
    ret |= writeRegister(ES7210_CLOCK_OFF_REG01, _clockRegValue);
    ret |= writeRegister(ES7210_POWER_DOWN_REG06, 0x00);
    ret |= writeRegister(ES7210_ANALOG_REG40, 0x43);
    ret |= writeRegister(ES7210_MIC1_POWER_REG47, 0x00);
    ret |= writeRegister(ES7210_MIC2_POWER_REG48, 0x00);
    ret |= writeRegister(ES7210_MIC3_POWER_REG49, 0x00);
    ret |= writeRegister(ES7210_MIC4_POWER_REG4A, 0x00);
    ret |= selectMic(_micSelect);
    
    if (ret == ESP_OK) {
        _started = true;
        Serial0.printf("[%s] ADC started successfully\n", TAG_ES7210);
    } else {
        Serial0.printf("[%s] ADC start failed\n", TAG_ES7210);
    }
    
    return ret;
}

// Stop ADC
esp_err_t ES7210::stop() {
    if (!_initialized) {
        Serial0.printf("[%s] Device not initialized\n", TAG_ES7210);
        return ESP_FAIL;
    }
    
    // Save current clock register value
    int regv = readRegister(ES7210_CLOCK_OFF_REG01);
    if (regv >= 0) {
        _clockRegValue = regv;
    }
    
    esp_err_t ret = ESP_OK;
    
    ret |= writeRegister(ES7210_MIC1_POWER_REG47, 0xff);
    ret |= writeRegister(ES7210_MIC2_POWER_REG48, 0xff);
    ret |= writeRegister(ES7210_MIC3_POWER_REG49, 0xff);
    ret |= writeRegister(ES7210_MIC4_POWER_REG4A, 0xff);
    ret |= writeRegister(ES7210_MIC12_POWER_REG4B, 0xff);
    ret |= writeRegister(ES7210_MIC34_POWER_REG4C, 0xff);
    ret |= writeRegister(ES7210_ANALOG_REG40, 0xc0);
    ret |= writeRegister(ES7210_CLOCK_OFF_REG01, 0x7f);
    ret |= writeRegister(ES7210_POWER_DOWN_REG06, 0x07);
    
    if (ret == ESP_OK) {
        _started = false;
        Serial0.printf("[%s] ADC stopped successfully\n", TAG_ES7210);
    } else {
        Serial0.printf("[%s] ADC stop failed\n", TAG_ES7210);
    }
    
    return ret;
}

// Set sample rate
esp_err_t ES7210::setSampleRate(es7210_sample_rate_t sampleRate) {
    uint32_t mclkFre = sampleRate * MCLK_DIV_FRE;
    int coeff = getCoeff(mclkFre, sampleRate);
    
    if (coeff < 0) {
        Serial0.printf("[%s] Unable to configure sample rate %dHz with %dHz MCLK\n", 
                     TAG_ES7210, sampleRate, mclkFre);
        return ESP_FAIL;
    }
    
    esp_err_t ret = ESP_OK;
    
    // Set adc_div & doubler & dll
    uint8_t regv = readRegister(ES7210_MAINCLK_REG02) & 0x00;
    regv |= coeffDiv[coeff].adc_div;
    regv |= coeffDiv[coeff].doubler << 6;
    regv |= coeffDiv[coeff].dll << 7;
    ret |= writeRegister(ES7210_MAINCLK_REG02, regv);
    
    // Set osr
    ret |= writeRegister(ES7210_OSR_REG07, coeffDiv[coeff].osr);
    
    // Set lrck
    ret |= writeRegister(ES7210_LRCK_DIVH_REG04, coeffDiv[coeff].lrck_h);
    ret |= writeRegister(ES7210_LRCK_DIVL_REG05, coeffDiv[coeff].lrck_l);
    
    if (ret == ESP_OK) {
        _sampleRate = sampleRate;
        Serial0.printf("[%s] Sample rate set to %dHz\n", TAG_ES7210, sampleRate);
    }
    
    return ret;
}

// Set bit depth
esp_err_t ES7210::setBits(es7210_bits_t bits) {
    uint8_t adcIface = readRegister(ES7210_SDP_INTERFACE1_REG11);
    adcIface &= 0x1f;
    
    switch (bits) {
        case ES7210_16_BITS:
            adcIface |= 0x60;
            break;
        case ES7210_24_BITS:
            adcIface |= 0x00;
            break;
        case ES7210_32_BITS:
            adcIface |= 0x80;
            break;
    }
    
    esp_err_t ret = writeRegister(ES7210_SDP_INTERFACE1_REG11, adcIface);
    
    if (ret == ESP_OK) {
        _bits = bits;
        Serial0.printf("[%s] Bit depth set to %d bits\n", TAG_ES7210, bits);
    }
    
    return ret;
}

// Set format
esp_err_t ES7210::setFormat(es7210_format_t format) {
    uint8_t adcIface = readRegister(ES7210_SDP_INTERFACE1_REG11);
    adcIface &= 0xfc;
    
    switch (format) {
        case ES7210_I2S_NORMAL:
            Serial0.printf("[%s] Format set to I2S\n", TAG_ES7210);
            adcIface |= 0x00;
            break;
        case ES7210_I2S_LEFT:
        case ES7210_I2S_RIGHT:
            Serial0.printf("[%s] Format set to Left Justified\n", TAG_ES7210);
            adcIface |= 0x01;
            break;
        case ES7210_I2S_DSP:
            if (I2S_DSP_MODE) {
                Serial0.printf("[%s] Format set to DSP-A\n", TAG_ES7210);
                adcIface |= 0x13;
            } else {
                Serial0.printf("[%s] Format set to DSP-B\n", TAG_ES7210);
                adcIface |= 0x03;
            }
            break;
    }
    
    esp_err_t ret = writeRegister(ES7210_SDP_INTERFACE1_REG11, adcIface);
    
    if (ret == ESP_OK) {
        _format = format;
    }
    
    return ret;
}

// Select microphones
esp_err_t ES7210::selectMic(es7210_input_mics_t mic) {
    esp_err_t ret = ESP_OK;
    
    if (!(mic & (ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2 | ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4))) {
        Serial0.printf("[%s] Invalid microphone selection: 0x%02X\n", TAG_ES7210, mic);
        return ESP_FAIL;
    }
    
    // Disable all mic gains first
    for (int i = 0; i < 4; i++) {
        ret |= updateRegisterBit(ES7210_MIC1_GAIN_REG43 + i, 0x10, 0x00);
    }
    ret |= writeRegister(ES7210_MIC12_POWER_REG4B, 0xff);
    ret |= writeRegister(ES7210_MIC34_POWER_REG4C, 0xff);
    
    // Enable selected microphones
    if (mic & ES7210_INPUT_MIC1) {
        Serial0.printf("[%s] Enabling MIC1\n", TAG_ES7210);
        ret |= updateRegisterBit(ES7210_CLOCK_OFF_REG01, 0x0b, 0x00);
        ret |= writeRegister(ES7210_MIC12_POWER_REG4B, 0x00);
        ret |= updateRegisterBit(ES7210_MIC1_GAIN_REG43, 0x10, 0x10);
    }
    if (mic & ES7210_INPUT_MIC2) {
        Serial0.printf("[%s] Enabling MIC2\n", TAG_ES7210);
        ret |= updateRegisterBit(ES7210_CLOCK_OFF_REG01, 0x0b, 0x00);
        ret |= writeRegister(ES7210_MIC12_POWER_REG4B, 0x00);
        ret |= updateRegisterBit(ES7210_MIC2_GAIN_REG44, 0x10, 0x10);
    }
    if (mic & ES7210_INPUT_MIC3) {
        Serial0.printf("[%s] Enabling MIC3\n", TAG_ES7210);
        ret |= updateRegisterBit(ES7210_CLOCK_OFF_REG01, 0x15, 0x00);
        ret |= writeRegister(ES7210_MIC34_POWER_REG4C, 0x00);
        ret |= updateRegisterBit(ES7210_MIC3_GAIN_REG45, 0x10, 0x10);
    }
    if (mic & ES7210_INPUT_MIC4) {
        Serial0.printf("[%s] Enabling MIC4\n", TAG_ES7210);
        ret |= updateRegisterBit(ES7210_CLOCK_OFF_REG01, 0x15, 0x00);
        ret |= writeRegister(ES7210_MIC34_POWER_REG4C, 0x00);
        ret |= updateRegisterBit(ES7210_MIC4_GAIN_REG46, 0x10, 0x10);
    }
    
    if (ret == ESP_OK) {
        _micSelect = mic;
        Serial0.printf("[%s] Microphones selected: 0x%02X\n", TAG_ES7210, mic);
    }
    
    return ret;
}

// Set gain
esp_err_t ES7210::setGain(es7210_gain_value_t gain) {
    const uint32_t maxGainValue = 14;
    
    if (gain > maxGainValue) {
        gain = static_cast<es7210_gain_value_t>(maxGainValue);
    }
    
    esp_err_t ret = ESP_OK;
    
    if (_micSelect & ES7210_INPUT_MIC1) {
        ret |= updateRegisterBit(ES7210_MIC1_GAIN_REG43, 0x0f, gain);
    }
    if (_micSelect & ES7210_INPUT_MIC2) {
        ret |= updateRegisterBit(ES7210_MIC2_GAIN_REG44, 0x0f, gain);
    }
    if (_micSelect & ES7210_INPUT_MIC3) {
        ret |= updateRegisterBit(ES7210_MIC3_GAIN_REG45, 0x0f, gain);
    }
    if (_micSelect & ES7210_INPUT_MIC4) {
        ret |= updateRegisterBit(ES7210_MIC4_GAIN_REG46, 0x0f, gain);
    }
    
    if (ret == ESP_OK) {
        _gain = gain;
        Serial0.printf("[%s] Gain set to %d (level %d)\n", TAG_ES7210, gain, gain * 3);
    }
    
    return ret;
}

// Get gain
int ES7210::getGain() {
    int regv = -1;
    
    if (_micSelect & ES7210_INPUT_MIC1) {
        regv = readRegister(ES7210_MIC1_GAIN_REG43);
    } else if (_micSelect & ES7210_INPUT_MIC2) {
        regv = readRegister(ES7210_MIC2_GAIN_REG44);
    } else if (_micSelect & ES7210_INPUT_MIC3) {
        regv = readRegister(ES7210_MIC3_GAIN_REG45);
    } else if (_micSelect & ES7210_INPUT_MIC4) {
        regv = readRegister(ES7210_MIC4_GAIN_REG46);
    } else {
        Serial0.printf("[%s] No microphone selected\n", TAG_ES7210);
        return -1;
    }
    
    if (regv < 0) {
        return -1;
    }
    
    return (regv & 0x0f);
}

// Set mute
esp_err_t ES7210::setMute(bool enable) {
    Serial0.printf("[%s] Mute %s\n", TAG_ES7210, enable ? "enabled" : "disabled");
    // TODO: Implement actual mute functionality if needed
    return ESP_OK;
}

// Check if device is present
bool ES7210::isPresent() {
    _wire->beginTransmission(_i2cAddr);
    uint8_t error = _wire->endTransmission();
    return (error == 0);
}

// Read register
int ES7210::readReg(uint8_t regAddr) {
    return readRegister(regAddr);
}

// Write register
esp_err_t ES7210::writeReg(uint8_t regAddr, uint8_t data) {
    return writeRegister(regAddr, data);
}

// Dump all registers
void ES7210::dumpRegisters() {
    Serial0.printf("[%s] === Register Dump ===\n", TAG_ES7210);
    for (int i = 0; i <= 0x4E; i++) {
        int reg = readRegister(i);
        if (reg >= 0) {
            Serial0.printf("REG[0x%02X]: 0x%02X\n", i, reg);
        }
    }
    Serial0.printf("[%s] =====================\n", TAG_ES7210);
}

// Print status
void ES7210::printStatus() {
    Serial0.printf("[%s] === Status ===\n", TAG_ES7210);
    Serial0.printf("I2C Address: 0x%02X\n", _i2cAddr);
    Serial0.printf("Initialized: %s\n", _initialized ? "Yes" : "No");
    Serial0.printf("Started: %s\n", _started ? "Yes" : "No");
    Serial0.printf("Sample Rate: %d Hz\n", _sampleRate);
    Serial0.printf("Bit Depth: %d bits\n", _bits);
    Serial0.printf("Format: %d\n", _format);
    Serial0.printf("Mode: %s\n", _mode == ES7210_MODE_MASTER ? "Master" : "Slave");
    Serial0.printf("Selected MICs: 0x%02X\n", _micSelect);
    Serial0.printf("Gain: %d\n", _gain);
    
    if (_initialized) {
        Serial0.printf("Power Reg (0x06): 0x%02X\n", readRegister(ES7210_POWER_DOWN_REG06));
        Serial0.printf("Clock Reg (0x01): 0x%02X\n", readRegister(ES7210_CLOCK_OFF_REG01));
        Serial0.printf("Mode Reg (0x08): 0x%02X\n", readRegister(ES7210_MODE_CONFIG_REG08));
    }
    Serial0.printf("[%s] =============\n", TAG_ES7210);
}

// Private methods implementation

esp_err_t ES7210::writeRegister(uint8_t regAddr, uint8_t data) {
    _wire->beginTransmission(_i2cAddr);
    _wire->write(regAddr);
    _wire->write(data);
    uint8_t error = _wire->endTransmission();
    
    if (error != 0) {
        Serial0.printf("[%s] Write reg 0x%02X failed: %d\n", TAG_ES7210, regAddr, error);
        return ESP_FAIL;
    }
    
    delay(1);
    return ESP_OK;
}

int ES7210::readRegister(uint8_t regAddr) {
    _wire->beginTransmission(_i2cAddr);
    _wire->write(regAddr);
    uint8_t error = _wire->endTransmission(false);
    
    if (error != 0) {
        Serial0.printf("[%s] Read reg 0x%02X failed: %d\n", TAG_ES7210, regAddr, error);
        return -1;
    }
    
    _wire->requestFrom(_i2cAddr, (uint8_t)1);
    if (_wire->available()) {
        return _wire->read();
    }
    return -1;
}

esp_err_t ES7210::updateRegisterBit(uint8_t regAddr, uint8_t updateBits, uint8_t data) {
    int regv = readRegister(regAddr);
    if (regv < 0) return ESP_FAIL;
    
    regv = (regv & (~updateBits)) | (updateBits & data);
    return writeRegister(regAddr, regv);
}

int ES7210::getCoeff(uint32_t mclk, uint32_t lrck) {
    for (size_t i = 0; i < coeffDivSize; i++) {
        if (coeffDiv[i].lrck == lrck && coeffDiv[i].mclk == mclk) {
            return i;
        }
    }
    return -1;
}

esp_err_t ES7210::resetChip() {
    esp_err_t ret = ESP_OK;
    ret |= writeRegister(ES7210_RESET_REG00, 0xff);
    delay(10);
    ret |= writeRegister(ES7210_RESET_REG00, 0x41);
    delay(10);
    return ret;
}

esp_err_t ES7210::powerUp() {
    return writeRegister(ES7210_POWER_DOWN_REG06, 0x00);
}

esp_err_t ES7210::powerDown() {
    return writeRegister(ES7210_POWER_DOWN_REG06, 0x07);
}