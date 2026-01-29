#ifndef GYRO_H
#define GYRO_H

#include <Arduino.h>
#include <MPU6050_6Axis_MotionApps20.h>

class Gyro : public MPU6050 {
public:
    uint8_t I2C_SDA_DEFAULT;
    uint8_t I2C_SCL_DEFAULT;
    
    // Raw Data storage
    int16_t RawGyroX, RawGyroY, RawGyroZ; 
    int16_t RawAccelX, RawAccelY, RawAccelZ; 
    
    // Processed Data
    float AngleX, AngleY, AngleZ; 
    float AccumAngleX, AccumAngleY, AccumAngleZ;
    
    bool dmpReady = false; 
    bool isCalibrated = false; 
    int stabilizationSampleCount = 0;
    
    // [NEW] Storage for Advanced Physics
    VectorInt16 aa;         // Raw Accel (Internal)
    VectorInt16 aaReal;     // Linear Accel (Gravity Removed)
    VectorFloat gravity;    // Gravity Vector
    
    Gyro(uint8_t I2C_SDA, uint8_t I2C_SCL);
    void setup();
    void setup(uint8_t I2C_SDA, uint8_t I2C_SCL);
    void getData();
    void readAll();
    
    // [NEW] Getters for the new data
    void getLinearAccel(int16_t &x, int16_t &y, int16_t &z);
    void getGravity(float &x, float &y, float &z);

    float readX();
    float readY();
    float readZ();
    float readAccumX();
    float readAccumY();
    float readAccumZ();
    
    void calibrate(int samples = 100); 
    void setCurrentAsZero();
    void resetAccumulation();
    void reset();
    
    bool isStabilized();
    void waitForStabilization();
    void resetStabilizationCheck();
    
private:
    static constexpr float STABILITY_THRESHOLD = 0.5;  
    static const unsigned long MIN_STABILIZATION_TIME = 3000; 
    static const int MIN_STABLE_SAMPLES = 50; 
    static const int STABILIZATION_CHECK_INTERVAL = 50; 
    
    unsigned long lastUpdate = 0;
    float angle[3];
    float zeroReference[3] = {0, 0, 0}; 
    float lastAngle[3] = {0, 0, 0}; 
    bool firstReading = true;
    
    uint16_t packetSize;
    uint8_t fifoBuffer[64];
    Quaternion q;
    
    unsigned long stabilizationStartTime = 0;
    float stabilizationBuffer[3] = {0, 0, 0}; 
    
    float wrapAngle(float angle);
    float angleDifference(float current, float previous);
};

#endif