#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "config.h"

class TempSensor {
private:
    Adafruit_ADS1115 ads;
    bool initialized;
    float lastTemp;
    float tempOffset;       // 温度校准偏移
    float tempBuffer[10];   // 用于滤波的缓冲区
    uint8_t bufferIndex;
    unsigned long lastSampleTime;

    // 将ADS1115的电压值转换为NTC温度
    float voltageToTemp(float voltage);
    
    // 移动平均滤波
    float applyFilter(float newTemp);

public:
    TempSensor();
    
    // 初始化温度传感器
    bool begin();
    
    // 读取当前温度
    float readTemperature();
    
    // 设置温度校准偏移
    void setCalibration(float offset);
    
    // 获取温度校准偏移
    float getCalibration();
    
    // 检查传感器状态
    bool checkSensor();
};

#endif // TEMP_SENSOR_H 