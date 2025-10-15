/**
 * @file CTP_FT6336.cpp
 * @brief FT6336 Touch Screen Driver
 * @version 
 * @date 
 * 
 * @copyright
 */
#pragma once
#include "Arduino.h"
#include "Wire.h"

/* Configs */
#define CTP_DEV_ADDR            0x38   // FT6336 I2C address
#define CTP_INT_PIN             -1     // INT pin not used (悬空)

#define CTP_HORIZON              // Setting for horizontal orientation

#ifdef CTP_HORIZON
    #define CTP_REVERS_XPOS     1
    #define CTP_REVERS_YPOS     0
    #define CTP_REVERS_XY       1
#else
#ifdef CTP_PORTRAIT
    #define CTP_REVERS_XPOS     0
    #define CTP_REVERS_YPOS     1
    #define CTP_REVERS_XY       0
#endif
#endif

#define USING_MAP_TO_CALIBRATE  0   // Option for calibration

class CTP {
    private:
        int _x_pos;
        int _y_pos;
        bool _enable;

        /* I2C related */
        TwoWire* _wire;
        uint8_t _dev_addr;

        inline void _I2C_init(TwoWire * wire, uint8_t dev_addr)
        {
            _wire = wire;
            _dev_addr = dev_addr;
        }

        inline bool _I2C_checkDevAvl()
        {
            _wire->beginTransmission(_dev_addr);
            return ( _wire->endTransmission() ? true : false);
        }

        inline void _I2C_write1Byte(uint8_t addr, uint8_t data)
        {
            _wire->beginTransmission(_dev_addr);
            _wire->write(addr);
            _wire->write(data);
            _wire->endTransmission();
        }

        inline void _I2C_readBuff(uint8_t addr, int size, uint8_t buff[])
        {
            _wire->beginTransmission(_dev_addr);
            _wire->write(addr);
            _wire->endTransmission();
            _wire->requestFrom(_dev_addr, (size_t)size);
            for (int i = 0; i < size; i++) {
                buff[i] = _wire->read();
            }
        }

        inline uint8_t _I2C_read8Bit(uint8_t addr)
        {
            _wire->beginTransmission(_dev_addr);
            _wire->write(addr);
            _wire->endTransmission();
            _wire->requestFrom(_dev_addr, (size_t)1);
            return _wire->read();
        }

        inline uint16_t _I2C_read12Bit(uint8_t addr)
        {
            uint8_t buff[2];
            _I2C_readBuff(addr, 2, buff);
            return (buff[0] << 4) + buff[1];
        }

        /* CTP init */
        inline void _init()
        {
            _reset_coor();
            _enable = true;

            // Disable auto-sleep on FT6336
            _I2C_write1Byte(0xFE, 0x07);
           printf("[CTP] Auto-sleep disabled");
        }

        inline void _reset_coor()
        {
            _x_pos = -1;
            _y_pos = -1;
        }

        inline void _update_coor()
        {
            uint8_t buff[4];
            _I2C_readBuff(0x03, 4, buff);

            #if CTP_REVERS_XY
                _y_pos = ((buff[0]&0x0F) << 8) | buff[1];
                _x_pos = ((buff[2]&0x0F) << 8) | buff[3];
                #if CTP_REVERS_XPOS
                    _x_pos = -( _x_pos ) + 320;
                #endif
                #if CTP_REVERS_YPOS
                    _y_pos = -( _y_pos ) + 240;
                #endif
            #else
                _x_pos = ((buff[0]&0x0F) << 8) | buff[1];
                _y_pos = ((buff[2]&0x0F) << 8) | buff[3];
            #endif
        }

    public:
        /**
         * @brief Initialize the CTP with a TwoWire object
         * 
         * @param wire I2C wire interface (e.g., Wire)
         */
        inline void init(TwoWire* wire)
        {
            _I2C_init(wire, CTP_DEV_ADDR);

            // Wait until device is available
            while (_I2C_checkDevAvl()) {
                printf("[CTP] Please touch screen to activate");
                delay(500);
            }
           printf("[CTP] Initialization successful");
            _init();
        }

        inline bool isTouched()
        {
            if (!_enable) return false;
            return _I2C_read8Bit(0x02) ? true : false;
        }

        inline void getPos(int& x_pos, int& y_pos)
        {
            _update_coor();
            x_pos = _x_pos;
            y_pos = _y_pos;

            #if USING_MAP_TO_CALIBRATE
                x_pos = map(_x_pos, 6, 258, 0, 280);
                y_pos = map(_y_pos, 15, 230, 0, 240);

                #ifdef CTP_HORIZON
                    x_pos = constrain(x_pos, 0, 280);
                    y_pos = constrain(y_pos, 0, 240);
                #else
                #ifdef CTP_PORTRAIT
                    x_pos = constrain(x_pos, 0, 240);
                    y_pos = constrain(y_pos, 0, 280);
                #endif
                #endif           
            #endif
        }

        inline void printCoordinate()
        {
            if (isTouched()) _update_coor();
            else _reset_coor();

            Serial0.printf("X:%03d Y:%03d\n", _x_pos, _y_pos);
        }

        inline void enable() { _enable = true; }
        inline void disable() { _enable = false; }
};
