#include "PCF8563.h"

bool PCF8563::begin() {
    if (!_wire) return false;
    _wire->beginTransmission(_addr);
    return (_wire->endTransmission() == 0);
}

bool PCF8563::getTime(RTC_Time &time) {
    if (!_wire) return false;
    _wire->beginTransmission(_addr);
    _wire->write(0x02); // Starting register：VL_seconds
    if (_wire->endTransmission(false) != 0) return false;
    if (_wire->requestFrom(_addr, (uint8_t)7) != 7) return false;

    uint8_t sec = _wire->read() & 0x7F;
    uint8_t min = _wire->read() & 0x7F;
    uint8_t hour = _wire->read() & 0x3F;
    uint8_t day = _wire->read() & 0x3F;
    uint8_t weekday = _wire->read() & 0x07;
    uint8_t month = _wire->read() & 0x1F;
    uint8_t year = _wire->read();

    time.sec = bcd2dec(sec);
    time.min = bcd2dec(min);
    time.hour = bcd2dec(hour);
    time.day = bcd2dec(day);
    time.weekday = bcd2dec(weekday);
    time.month = bcd2dec(month);
    time.year = 2000 + bcd2dec(year);

    return true;
}

bool PCF8563::setTime(const RTC_Time &time) {
    if (!_wire) return false;
    _wire->beginTransmission(_addr);
    _wire->write(0x02); // Starting register
    _wire->write(dec2bcd(time.sec) & 0x7F);
    _wire->write(dec2bcd(time.min) & 0x7F);
    _wire->write(dec2bcd(time.hour) & 0x3F);
    _wire->write(dec2bcd(time.day) & 0x3F);
    _wire->write(dec2bcd(time.weekday) & 0x07);
    _wire->write(dec2bcd(time.month) & 0x1F);
    _wire->write(dec2bcd(time.year % 100));
    return (_wire->endTransmission() == 0);
}

uint8_t PCF8563::bcd2dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

uint8_t PCF8563::dec2bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}