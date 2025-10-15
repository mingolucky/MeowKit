#include "QMI8658A.h"
#include <math.h>
#include <Wire.h>

QMI8658A::QMI8658A() : _addr(QMI8658A_ADDR) {}

bool QMI8658A::begin(uint8_t addr) {
    _addr = addr;

    // read WHO_AM_I
    uint8_t id = readRegister(QMI8658A_WHO_AM_I);
    uint8_t tryCount = 10;
    while (id != 0x05 && tryCount--) {
        delay(100);
        id = readRegister(QMI8658A_WHO_AM_I);
    }
    if (id != 0x05) {
        Serial.println("[QMI8658A] WHO_AM_I check failed!");
        return false;
    }
    Serial.println("[QMI8658A] WHO_AM_I check passed!");

    // Initialize registers
    writeRegister(QMI8658A_RESET, 0xB0);  // reset
    delay(10);
    writeRegister(QMI8658A_CTRL1, 0x40);  // Address auto-increment
    writeRegister(QMI8658A_CTRL7, 0x03);  // Enable accelerometer and gyroscope
    writeRegister(QMI8658A_CTRL2, 0x95);  // ACC 4g 250Hz
    writeRegister(QMI8658A_CTRL3, 0xD5);  // GYRO 512dps 250Hz
    delay(10);

    return true;
}

void QMI8658A::reset() {
    writeRegister(QMI8658A_RESET, 0xB0);
    delay(10);
}

bool QMI8658A::readRawData(QMI8658A_Data* data) {
    // Check if the data is ready
    uint8_t status = readRegister(QMI8658A_STATUS0);
    if ((status & 0x03) == 0) {
        return false;
    }

    uint8_t buf[12] = {0};
    readMultiple(QMI8658A_AX_L, buf, 12);

    data->acc_x = (int16_t)((buf[1] << 8) | buf[0]);
    data->acc_y = (int16_t)((buf[3] << 8) | buf[2]);
    data->acc_z = (int16_t)((buf[5] << 8) | buf[4]);
    data->gyr_x = (int16_t)((buf[7] << 8) | buf[6]);
    data->gyr_y = (int16_t)((buf[9] << 8) | buf[8]);
    data->gyr_z = (int16_t)((buf[11] << 8) | buf[10]);

    // Unit conversion
    // Acceleration: 4g range,16bit，1LSB=4g/32768=0.000122g
    data->acc_x_g = data->acc_x * 4.0f / 32768.0f;
    data->acc_y_g = data->acc_y * 4.0f / 32768.0f;
    data->acc_z_g = data->acc_z * 4.0f / 32768.0f;
    // Gyroscope: 512dps range，16bit，1LSB=512/32768=0.015625dps
    data->gyr_x_dps = data->gyr_x * 512.0f / 32768.0f;
    data->gyr_y_dps = data->gyr_y * 512.0f / 32768.0f;
    data->gyr_z_dps = data->gyr_z * 512.0f / 32768.0f;

    return true;
}

void QMI8658A::calibrateGyroBias(int samples) {
    float sum_x = 0, sum_z = 0;
    QMI8658A_Data data;
    for (int i = 0; i < samples; ++i) {
        if (readRawData(&data)) {
            sum_x += data.gyr_x;
            sum_z += data.gyr_z;
        }
        delay(2);
    }
    gyro_bias_x = sum_x / samples;
    gyro_bias_z = sum_z / samples;
}

void QMI8658A::filterGyro(QMI8658A_Data* data) {
    // First-order low-pass filtering to prevent jitter
    static float last_gyr_x = 0, last_gyr_y = 0, last_gyr_z = 0;
    data->gyr_x = 0.7f * last_gyr_x + 0.3f * data->gyr_x;
    data->gyr_y = 0.7f * last_gyr_y + 0.3f * data->gyr_y;
    data->gyr_z = 0.7f * last_gyr_z + 0.3f * data->gyr_z;
    last_gyr_x = data->gyr_x;
    last_gyr_y = data->gyr_y;
    last_gyr_z = data->gyr_z;
    // Synchronous floating point
    data->gyr_x_dps = data->gyr_x * 512.0f / 32768.0f;
    data->gyr_y_dps = data->gyr_y * 512.0f / 32768.0f;
    data->gyr_z_dps = data->gyr_z * 512.0f / 32768.0f;
}

void QMI8658A::integrateGyro(QMI8658A_Data* data) {
    unsigned long now = millis();
    float dt = (lastUpdateTime > 0) ? (now - lastUpdateTime) / 1000.0f : 0.0f;
    lastUpdateTime = now;
    // Remove zero drift
    float gx = (data->gyr_x - gyro_bias_x) * 512.0f / 32768.0f;
    float gy = (data->gyr_y - gyro_bias_y) * 512.0f / 32768.0f;
    float gz = (data->gyr_z - gyro_bias_z) * 512.0f / 32768.0f;
    integratedAngleX += gx * dt;
    integratedAngleY += gy * dt;
    integratedAngleZ += gz * dt;
    data->angle_x = integratedAngleX;
    data->angle_y = integratedAngleY;
    data->angle_z = integratedAngleZ;
}

void QMI8658A::updateMouseDelta(QMI8658A_Data* data, float sensitivity) {
    static float last_angle_x = 0.0f;
    static float last_angle_y = 0.0f;
    static float last_angle_z = 0.0f;

    filterGyro(data);
    integrateGyro(data);

    // Calculate the increment of this integral angle
    float delta_x = data->angle_x - last_angle_x;
    float delta_y = data->angle_y - last_angle_y;
    float delta_z = data->angle_z - last_angle_z;
    last_angle_x = data->angle_x;
    last_angle_y = data->angle_y;
    last_angle_z = data->angle_z;

    // dx is the increase (e.g. 2 times), dy is the reverse correction
    data->mouse_dx = static_cast<int>(-delta_z * sensitivity * 3.0f); // Zoom dx
    data->mouse_dy = static_cast<int>(-delta_y * sensitivity * 3.0f);          // Correct dy direction

    // Limit the maximum movement to prevent jumps
    const int max_move = 20;
    if (data->mouse_dx > max_move) data->mouse_dx = max_move;
    if (data->mouse_dx < -max_move) data->mouse_dx = -max_move;
    if (data->mouse_dy > max_move) data->mouse_dy = max_move;
    if (data->mouse_dy < -max_move) data->mouse_dy = -max_move;
}

bool QMI8658A::autoSelfTestAndCalibrate() {
    // 1. Disable Sensor
    writeRegister(QMI8658A_CTRL7, 0x00);
    delay(10);

    // 2. Configure self-test parameters (take acceleration self-test as an example, the same applies to gyroscope)
    // Setting ODR and aST
    uint8_t ctrl2 = readRegister(QMI8658A_CTRL2);
    ctrl2 |= (1 << 7); // aST bit7=1
    writeRegister(QMI8658A_CTRL2, ctrl2);

    // 3. Triggering self-test
    // Set INT2 or STATUSINT.bit0 = 1 and wait for the self-test to complete
    // Here we only do simple delay, interrupt or polling can be used
    delay(30);

    // 4. Reading self-test output
    uint8_t buf[6];
    readMultiple(0x51, buf, 6); // 0x51~0x56: dVX_L/H, dVY_L/H, dVZ_L/H
    int16_t dVX = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t dVY = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t dVZ = (int16_t)((buf[5] << 8) | buf[4]);

    // 5. Determine the self-test result (acceleration self-test threshold ±200mg, 16-bit signature, 0.5mg/LSB)
    float dVX_g = dVX * 0.0005f;
    float dVY_g = dVY * 0.0005f;
    float dVZ_g = dVZ * 0.0005f;
    bool pass = (fabs(dVX_g) > 0.2f) && (fabs(dVY_g) > 0.2f) && (fabs(dVZ_g) > 0.2f);

    // 6. Restore user configuration
    ctrl2 &= ~(1 << 7); // Close aST
    writeRegister(QMI8658A_CTRL2, ctrl2);
    writeRegister(QMI8658A_CTRL7, 0x03); // Re-enable

    // 7. Automatic zero drift calibration
    if (pass) {
        calibrateGyroBias(200);
    }

    return pass;
}

void QMI8658A::complementaryFilter(QMI8658A_Data* data, float alpha) {
    unsigned long now = millis();
    float dt = (lastUpdateTime > 0) ? (now - lastUpdateTime) / 1000.0f : 0.0f;
    lastUpdateTime = now;

    // 1. Angular velocity (to remove drift, in dps)
    float gx = (data->gyr_x - gyro_bias_x) * 512.0f / 32768.0f;
    float gy = (data->gyr_y - gyro_bias_y) * 512.0f / 32768.0f;
    float gz = (data->gyr_z - gyro_bias_z) * 512.0f / 32768.0f;

    // 2. Acceleration angle (radians)
    float acc_pitch = atan2(-data->acc_x_g, sqrt(data->acc_y_g * data->acc_y_g + data->acc_z_g * data->acc_z_g)) * 180.0f / M_PI;
    float acc_roll  = atan2(data->acc_y_g, data->acc_z_g) * 180.0f / M_PI;

    // 3. Euler angle integral (gyroscope integral, unit degree)
    pitch += gy * dt;
    roll  += gx * dt;
    yaw   += gz * dt;

    // --- New: yaw limiter to prevent infinite accumulation ---
    if (yaw > 180.0f)  yaw -= 360.0f;
    if (yaw < -180.0f) yaw += 360.0f;

    // 4. Complementary filter fusion
    pitch = alpha * pitch + (1 - alpha) * acc_pitch;
    roll  = alpha * roll  + (1 - alpha) * acc_roll;
    // yaw Gyroscope integration only (no magnetometer)

    // 5. Output
    data->pitch = pitch;
    data->roll  = roll;
    data->yaw   = yaw;
}

// --- I2C low-level operations ---
uint8_t QMI8658A::readRegister(uint8_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

void QMI8658A::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void QMI8658A::readMultiple(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, len);
    for (uint8_t i = 0; i < len && Wire.available(); ++i) {
        buf[i] = Wire.read();
    }
}