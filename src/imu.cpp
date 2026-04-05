#include <M5Unified.h>
#include "imu.h"
#include "esp_log.h"

static const char* TAG = "IMU";

IMU::IMU() : initialized(false) {
    accelOffset = {0, 0, 0};
    gyroOffset = {0, 0, 0};
}

IMU::~IMU() {}

bool IMU::init(uint8_t sda_pin, uint8_t scl_pin) {
    // M5StickS3 specific I2C pins are GPIO 47 (SDA) and GPIO 48 (SCL).
    // NOTE: M5.begin() already initializes I2C, so we don't reinitialize it.
    // Attempting to reinitialize causes I2C communication issues.
    if (sda_pin == 0 || scl_pin == 0) {
        sda_pin = 47;
        scl_pin = 48;
    }

    ESP_LOGI(TAG, "Initializing M5Unified IMU (SDA=%d, SCL=%d)", sda_pin, scl_pin);
    
    unsigned long initStart = millis();
    // DO NOT call M5.In_I2C.begin() - it's already done by M5.begin()
    // M5.In_I2C.begin(sda_pin, scl_pin, 100000U);
    
    // Poll IMU multiple times to ensure it's responding
    for (int i = 0; i < 5; i++) {
        M5.update();
        M5.Imu.update();
        delay(10);
    }

    if (!M5.Imu.isEnabled()) {
        ESP_LOGE(TAG, "IMU initialization failed.");
        initialized = false;
        return false;
    }
    initialized = true;
    ESP_LOGI(TAG, "M5Unified IMU initialized successfully (took %lumsms)", millis() - initStart);
    return true;
}

IMU::AccelData IMU::readAccel() {
    AccelData data = {0, 0, 0};
    
    if (!initialized) {
        return data;
    }
    
    // M5.update() must be called to refresh sensor data from I2C
    M5.update();
    M5.Imu.update();
    
    M5.Imu.getAccel(&data.x, &data.y, &data.z);
    
    data.x -= accelOffset.x;
    data.y -= accelOffset.y;
    data.z -= accelOffset.z;
    
    return data;
}

IMU::GyroData IMU::readGyro() {
    GyroData data = {0, 0, 0};
    
    if (!initialized) {
        return data;
    }
    
    // M5.update() must be called to refresh sensor data from I2C
    M5.update();
    M5.Imu.update();
    M5.Imu.getGyro(&data.x, &data.y, &data.z);
    data.x -= gyroOffset.x;
    data.y -= gyroOffset.y;
    data.z -= gyroOffset.z;
    
    return data;
}

void IMU::resetCalibration() {
    accelOffset = {0, 0, 0};
    gyroOffset = {0, 0, 0};
    ESP_LOGI(TAG, "Calibration reset");
}

float IMU::getAccelMagnitude() {
    AccelData data = readAccel();
    return sqrtf(data.x * data.x + data.y * data.y + data.z * data.z);
}
