#include "temp_sensor.h"
#include <math.h>

TempSensor::TempSensor() {
    initialized = false;
    lastTemp = 0.0f;
    tempOffset = 0.0f;
    bufferIndex = 0;
    lastSampleTime = 0;
    
    // 初始化温度缓冲区
    for (int i = 0; i < 10; i++) {
        tempBuffer[i] = 0.0f;
    }
}

bool TempSensor::begin() {
    // 初始化ADS1115
    if (!ads.begin(ADS1115_ADDR)) {
        Serial.println("无法初始化ADS1115");
        return false;
    }
    
    // 设置增益 - 默认±2.048V
    ads.setGain(GAIN_ONE);
    
    // 设置数据速率 - 使用正确的常量值
    // 注：直接使用值 0x4 对应128SPS (参考Adafruit_ADS1X15.h文档)
    ads.setDataRate(0x4);
    
    initialized = true;
    Serial.println("ADS1115初始化成功");
    
    // 读取初始温度值并填充缓冲区
    float initialTemp = readTemperature();
    for (int i = 0; i < 10; i++) {
        tempBuffer[i] = initialTemp;
    }
    
    return true;
}

float TempSensor::voltageToTemp(float voltage) {
    // 计算NTC电阻值
    float ntcR = NTC_SERIES_R * (NTC_VCC / voltage - 1.0f);
    
    // 使用B值方程计算温度 (开尔文)
    float steinhart = log(ntcR / NTC_R25) / NTC_B;
    steinhart += 1.0f / (25.0f + 273.15f);
    steinhart = 1.0f / steinhart;
    
    // 转换为摄氏度
    float tempC = steinhart - 273.15f;
    
    // 应用校准偏移
    tempC += tempOffset;
    
    return tempC;
}

float TempSensor::applyFilter(float newTemp) {
    // 存储新温度到缓冲区
    tempBuffer[bufferIndex] = newTemp;
    bufferIndex = (bufferIndex + 1) % 10;
    
    // 计算移动平均
    float sum = 0.0f;
    for (int i = 0; i < 10; i++) {
        sum += tempBuffer[i];
    }
    
    return sum / 10.0f;
}

float TempSensor::readTemperature() {
    if (!initialized) {
        return -999.0f; // 错误值
    }
    
    // 检查采样间隔
    unsigned long now = millis();
    if (now - lastSampleTime < TEMP_SAMPLE_INTERVAL && lastSampleTime > 0) {
        return lastTemp;
    }
    
    lastSampleTime = now;
    
    // 读取ADS1115
    int16_t adc = ads.readADC_SingleEnded(NTC_CHANNEL);
    
    // 转换为电压
    float voltage = ads.computeVolts(adc);
    
    // 转换为温度
    float rawTemp = voltageToTemp(voltage);
    
    // 应用滤波
    float filteredTemp = applyFilter(rawTemp);
    
    // 存储为最后一次读数
    lastTemp = filteredTemp;
    
    return filteredTemp;
}

void TempSensor::setCalibration(float offset) {
    tempOffset = offset;
}

float TempSensor::getCalibration() {
    return tempOffset;
}

bool TempSensor::checkSensor() {
    if (!initialized) {
        return false;
    }
    
    // 读取当前温度
    float temp = readTemperature();
    
    // 检查温度是否在合理范围内
    if (temp < TEMP_PROTECTION_MIN - 10.0f || temp > TEMP_PROTECTION_MAX + 10.0f) {
        return false;
    }
    
    return true;
} 