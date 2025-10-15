#include "devices.h"

DEVICES devices;

IRrecv irrecv(HAL_PIN_IR_RX);
IRsend irsend(HAL_PIN_IR_TX);

// Remove duplicate global variables and use class members
decode_results results;

void DEVICES::init()
{
    Serial0.begin(115200);
    Serial0.println("BSP init...");

    /* Init POWER */
    button.PWR_ON.begin();
    // At the early stage of power-on, the system detects whether the key is pressed continuously for >= 2000ms and then latches the power supply.
    {
        const uint16_t hold_ms = 1500;      // Long press judgment time
        const uint32_t window_ms = 3000;    // Maximum waiting window to avoid stuck
        uint32_t start_ms = millis();
        bool latched = false;
        while (millis() - start_ms < window_ms) {
            button.PWR_ON.read();           // Debounce + Status Update
            if (button.PWR_ON.held(hold_ms)) {
                pinMode(HAL_PIN_PWR_HOLD, OUTPUT);
                digitalWrite(HAL_PIN_PWR_HOLD, HIGH); // Latch power supply
                latched = true;
                break;
            }
            delay(5);
        }
        if (latched) delay(50); // Give the power lock some time
    }

    /* Init I2C */
    Wire.begin(HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL);
    Wire.setClock(I2CSPEED);
    delay(200); // Increase delay to ensure stable power-on of I2C devices

    /* Init PCA9557 */
    #define LCD_CS_PIN (0)
    io.pinMode(LCD_CS_PIN, OUTPUT);
    io.digitalWrite(LCD_CS_PIN, LOW); //LCD enable
    #define PA_EN (1)
    io.pinMode(PA_EN, OUTPUT);
    io.digitalWrite(PA_EN, HIGH); // Speaker Enable

    /* I2C device detection */
    struct {
        const char* name;
        uint8_t addr;
        uint8_t* result;
    } i2c_devs[] = {
        {"PCA9557", 0x19, nullptr},
        {"RTC", PCF8563_ADDR, nullptr},
        {"IMU", QMI8658A_ADDR, nullptr},
        {"CTP", CTP_DEV_ADDR, nullptr},
        {"SPEAKER", ES8311_ADDR, nullptr},
        {"MIC", ES7210_ADDR, nullptr}
    };
    constexpr size_t i2c_dev_count = sizeof(i2c_devs) / sizeof(i2c_devs[0]);
    uint8_t i2c_results[i2c_dev_count];

    /* Init lcd bootloader */
    Lcd.init();
    Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    Lcd.setFont(&fonts::efontCN_16);
    Lcd.setCursor(0, 0);
    Lcd.printf(" BSP %s :)\n Author: Mingo@whitecliff\n", BSP_VERISON);
    //Lcd.printf(" Project: %s\n", PROJECT_NAME);

    // I2C device detection (with retry)
    for (size_t i = 0; i < i2c_dev_count; ++i) {
        uint8_t result = 1;
        for (int retry = 0; retry < 3; ++retry) {
            Wire.beginTransmission(i2c_devs[i].addr);
            result = Wire.endTransmission();
            if (result == 0) break; // Exit and try again if device is detected
            delay(10);
        }
        i2c_results[i] = result;
        i2c_devs[i].result = &i2c_results[i];
    }

    // Display I2C detection results
    for (size_t i = 0; i < sizeof(i2c_devs)/sizeof(i2c_devs[0]); ++i) {
        if (*i2c_devs[i].result == 0) {
            Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
            Lcd.printf(" %s: OK\n", i2c_devs[i].name);
        } else {
            Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
            Lcd.printf(" %s: FAIL\n", i2c_devs[i].name);
        }
    }
    
    // Display SD card status
    Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    sd.init();
    if (sd.isInited()) {
        Lcd.printf(" SD Card: OK");
        
        // Get SD card type - use SD_MMC API directly
        uint8_t cardType = SD_MMC.cardType();
        const char* typeStr;
        switch(cardType) {
            case CARD_MMC:
                typeStr = "MMC";
                break;
            case CARD_SD:
                typeStr = "SD";
                break;
            case CARD_SDHC:
                typeStr = "SDHC";
                break;
            default:
                typeStr = "UNKNOWN";
                break;
        }
        
        // Get SD card capacity - directly use SD_MMC API
        uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024); //Convert to MB
        
        // Print type and capacity
        if (cardSize >= 1024) {
            float sizeGB = cardSize / 1024.0f;
            Lcd.printf(" [%s %.1fGB]\n", typeStr, sizeGB);
        } else {
            Lcd.printf(" [%s %lluMB]\n", typeStr, cardSize);
        }
    } else {
        Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        Lcd.printf(" SD Card: FAIL\n");
        Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    
    // Restore default color after checking
    Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    Lcd.printf(" Battery: %d%%\n", getBatteryPercent());
    pcf.begin();
    RTC_Time t = {0, 24, 0, 21, 4, 8, 2025}; 
    pcf.setTime(t);
    // Record the current cursor position for easy refresh
    int info_y = Lcd.getCursorY();

    // Time display is refreshed within 3 seconds
    unsigned long start = millis();
    while (millis() - start < 3000) {
        RTC_Time now;
        pcf.getTime(now);
        Lcd.setCursor(0, info_y);
        Lcd.printf(" Time: %04d-%02d-%02d %02d:%02d:%02d   \n",
            now.year, now.month, now.day, now.hour, now.min, now.sec);
        delay(500); //Refresh every 0.5 seconds
    }
    /* Init button A and B */
    button.A.begin();
    button.B.begin();
    /* IR_T Close */ 
    pinMode(HAL_PIN_IR_TX, OUTPUT);
    digitalWrite(HAL_PIN_IR_TX, LOW);
    
    // Initialize the infrared transmitter
    irsend.begin();
    delay(1000); // Wait for the infrared transmitter to stabilize

    // Detect A button and send IR
    button.A.read();
    Lcd.printf(" Please press button A...");
    while (true) {
        button.A.read();
        if (button.A.pressed()) {
        Lcd.printf(" A OK\n");
            Serial0.println("NEC");
            // irsend.sendNEC(0x00FFE01FUL);
            // delay(2000);
            break;
        }
        delay(5);
    }

    // Detect B button
    button.B.read();
    Lcd.printf(" Please press button B...");
    while (true) {
        button.B.read();
        if (button.B.pressed()) {
        Lcd.printf(" B OK\n");
            break;
        }
        delay(5);
    }

    // Waiting for infrared signal reception
    // Lcd.printf(" Waiting for infrared signal...");
    // irrecv.enableIRIn(); // Start infrared reception
    // delay(1000); // Wait for the infrared receiver to stabilize
    // while (true) {
    //     if (irrecv.decode(&results)) {
    //         Serial0.printf("0x%llX\n", results.value);
    //         irrecv.resume();  // Receive the next value
    //         Lcd.printf("\rtake overOK\n");
    //         break;
    //     }
    //     delay(10);
    // }

    Lcd.printf(" BSP init done!\n");
    delay(2000); // Wait for the display to stabilize

}

int DEVICES::getBatteryPercent()
{
    
    const float R7 = 100000.0f; // 100k
    const float R8 = 100000.0f; // 100k
    const float adcMax = 4095.0f;
    const float vRef = 3.3f; // ESP32 ADC reference voltage

    int raw = analogRead(HAL_adcPin);
    float vDiv = (raw / adcMax) * vRef;
    float vBat = vDiv * (R7 + R8) / R8;

    float percent = 0;
    if (vBat >= 4.20f)
        percent = 100;
    else if (vBat >= 4.10f)
        percent = 90 + (vBat - 4.10f) * 100;
    else if (vBat >= 3.95f)
        percent = 80 + (vBat - 3.95f) * 66.7f;
    else if (vBat >= 3.80f)
        percent = 60 + (vBat - 3.80f) * 133.3f;
    else if (vBat >= 3.65f)
        percent = 40 + (vBat - 3.65f) * 133.3f;
    else if (vBat >= 3.50f)
        percent = 20 + (vBat - 3.50f) * 133.3f;
    else if (vBat >= 3.30f)
        percent = 5 + (vBat - 3.30f) * 75.0f;
    else
        percent = 0;

    if (percent > 100.0f) percent = 100.0f;
    if (percent < 0.0f) percent = 0.0f;
    return (int)percent;
}