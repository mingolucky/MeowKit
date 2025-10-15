#pragma once

/**
 * @brief Pin configs
 *
 */
/* LCD */
#define HAL_PIN_LCD_MOSI 40
#define HAL_PIN_LCD_MISO -1
#define HAL_PIN_LCD_SCLK 41
#define HAL_PIN_LCD_DC 39
#define HAL_PIN_LCD_CS -1
#define HAL_PIN_LCD_RST -1
#define HAL_PIN_LCD_BUSY -1
#define HAL_PIN_LCD_BL 42

/* Power */
#define HAL_PIN_PWR_HOLD 11//high：Power on
#define HAL_PIN_PWR_ON 10 

#define HAL_adcPin 6

/* Button */
#define HAL_A 5
#define HAL_B 4

/* SD card 3-wire SDMMC 1-bit mode */
#define SDMMC_SD_PIN_CLK   47
#define SDMMC_SD_PIN_CMD   48
#define SDMMC_SD_PIN_D0    21

/* I2C */
#define HAL_PIN_I2C_SCL 2
#define HAL_PIN_I2C_SDA 1
#define I2CSPEED        400000 // Clock Rate

/* I2S */
#define MCLKPIN             38 // Master Clock
#define BCLKPIN             14 // Bit Clock
#define WSPIN               13 // Word select
#define DOPIN               45 // Data Out
#define DIPIN               12 // Data In

// To maintain compatibility, add the following alias definition
#define HAL_PIN_I2S_BCLK    BCLKPIN
#define HAL_PIN_I2S_WS      WSPIN
#define HAL_PIN_I2S_DOUT    DOPIN
#define HAL_PIN_I2S_DIN     DIPIN
#define HAL_PIN_I2S_MCLK    MCLKPIN

/* IR */
#define HAL_PIN_IR_TX 15 // IR TX pin
#define HAL_PIN_IR_RX 16 // IR RX pin

/* Common configs */
#define BSP_VERISON                "v1.0"
#define PROJECT_NAME               "MeowKit"

/* FFT and Audio Analysis Settings */
#define FFT_SAMPLES         1024    // Number of FFT samples (must be a power of 2)
#define SAMPLE_RATE         44100   // Sampling rate
#define VU_UPDATE_RATE      50      // VU meter update frequency (ms)
#define AUDIO_BUFFER_SIZE   512     // Audio buffer size

/* App Enable/Disable Settings */
#define ENABLE_BADUSB_APP   1       // 1: Enable BadUSB, 0: Disable BadUSB