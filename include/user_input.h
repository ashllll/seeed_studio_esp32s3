#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include "config.h"

// 编码器事件
enum EncoderEvent {
    EV_NONE = 0,
    EV_SINGLE_CLICK,
    EV_DOUBLE_CLICK,
    EV_LONG_PRESS,
    EV_ROTATE_CW,      // 顺时针旋转
    EV_ROTATE_CCW      // 逆时针旋转
};

class UserInput {
private:
    AiEsp32RotaryEncoder encoder;
    bool initialized;
    
    // 按钮状态
    bool buttonPressed;
    unsigned long buttonPressTime;
    unsigned long lastButtonReleaseTime;
    uint8_t clickCount;
    
    // 长按和双击检测配置
    const unsigned long LONG_PRESS_TIME = 1000;  // 长按时间 (毫秒)
    const unsigned long DOUBLE_CLICK_TIME = 500; // 双击间隔 (毫秒)
    
    // 编码器计数和步长
    int16_t encoderValue;
    uint8_t encoderStep;
    
    // 处理单击、双击、长按的标志位
    bool processingLongPress;
    bool processingDoubleClick;
    
public:
    UserInput();
    
    // 初始化用户输入
    bool begin();
    
    // 更新用户输入状态
    void update();
    
    // 获取编码器事件
    EncoderEvent getEvent();
    
    // 获取编码器当前值
    int16_t getEncoderValue();
    
    // 设置编码器当前值
    void setEncoderValue(int16_t value);
    
    // 设置编码器步长
    void setEncoderStep(uint8_t step);
    
    // 获取编码器步长
    uint8_t getEncoderStep();
    
    // 按钮中断处理
    static void IRAM_ATTR handleButtonInterrupt();
    
    // 编码器中断处理
    static void IRAM_ATTR handleEncoderInterrupt();
};

#endif // USER_INPUT_H 