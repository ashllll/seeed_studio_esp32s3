#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <Arduino.h>
#include "config.h"

class PWMController {
private:
    bool initialized;
    uint16_t dutyCycle;   // 占空比 (0-1023)
    bool enabled;         // 是否启用输出
    
public:
    PWMController();
    
    // 初始化PWM输出
    bool begin();
    
    // 设置PWM占空比
    void setDutyCycle(uint16_t duty);
    
    // 获取当前占空比
    uint16_t getDutyCycle();
    
    // 获取当前功率百分比
    uint8_t getPowerPercentage();
    
    // 启用PWM输出
    void enable();
    
    // 禁用PWM输出
    void disable();
    
    // 是否已启用
    bool isEnabled();
    
    // 紧急停止 (立即断开输出)
    void emergencyStop();
};

#endif // PWM_CONTROLLER_H 