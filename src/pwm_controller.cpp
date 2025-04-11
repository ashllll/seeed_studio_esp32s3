#include "pwm_controller.h"

PWMController::PWMController() {
    initialized = false;
    dutyCycle = 0;
    enabled = false;
}

bool PWMController::begin() {
    // 配置LEDC通道
    // ESP32 LEDC配置: 通道, 频率, 分辨率
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    
    // 将引脚与LEDC通道关联
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
    
    // 初始化PWM输出为0 (关闭)
    setDutyCycle(0);
    
    initialized = true;
    enabled = false;
    
    Serial.println("PWM控制器初始化成功");
    return true;
}

void PWMController::setDutyCycle(uint16_t duty) {
    // 限制占空比范围
    if (duty > (1 << PWM_RESOLUTION) - 1) {
        duty = (1 << PWM_RESOLUTION) - 1;
    }
    
    dutyCycle = duty;
    
    // 仅在启用状态下更新实际输出
    if (enabled && initialized) {
        ledcWrite(PWM_CHANNEL, dutyCycle);
    }
}

uint16_t PWMController::getDutyCycle() {
    return dutyCycle;
}

uint8_t PWMController::getPowerPercentage() {
    // 将10位分辨率 (0-1023) 转换为百分比 (0-100)
    return (uint8_t)((dutyCycle * 100) / ((1 << PWM_RESOLUTION) - 1));
}

void PWMController::enable() {
    if (!initialized) {
        return;
    }
    
    enabled = true;
    ledcWrite(PWM_CHANNEL, dutyCycle);
    
    Serial.println("PWM输出已启用");
}

void PWMController::disable() {
    if (!initialized) {
        return;
    }
    
    enabled = false;
    ledcWrite(PWM_CHANNEL, 0); // 将输出设为0
    
    Serial.println("PWM输出已禁用");
}

bool PWMController::isEnabled() {
    return enabled;
}

void PWMController::emergencyStop() {
    // 紧急情况下立即关闭输出
    if (initialized) {
        ledcWrite(PWM_CHANNEL, 0);
        enabled = false;
        dutyCycle = 0;
        
        Serial.println("PWM紧急停止!");
    }
} 