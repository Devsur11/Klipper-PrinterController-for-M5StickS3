#ifndef IMU_H
#define IMU_H

#include <Arduino.h>
#include <M5Unified.h>

class IMU {
public:
    struct AccelData {
        float x;  // m/s²
        float y;  // m/s²
        float z;  // m/s²
    };

    struct GyroData {
        float x;   // deg/s
        float y;   // deg/s
        float z;   // deg/s
    };

    // Singleton pattern
    static IMU& getInstance() {
        static IMU instance;
        return instance;
    }

    // Initialize IMU with custom SDA/SCL pins
    bool init(uint8_t sda_pin = 47, uint8_t scl_pin = 48);
    
    // Read accelerometer data
    AccelData readAccel();
    
    // Read gyroscope data
    GyroData readGyro();
    
    // Get calibrated acceleration magnitude
    float getAccelMagnitude();
    
    // Check if IMU is initialized
    bool isInitialized() const { return initialized; }
    
    // Reset calibration data
    void resetCalibration();

private:
    IMU();
    ~IMU();
    
    bool initialized;
    
    // Calibration offsets
    AccelData accelOffset;
    GyroData gyroOffset;
};

#endif
