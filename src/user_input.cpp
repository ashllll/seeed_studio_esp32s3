#include "user_input.h"

// 全局变量用于中断处理
static volatile bool g_buttonChanged = false;
static AiEsp32RotaryEncoder* g_encoder = nullptr;

UserInput::UserInput() 
    : encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BTN_PIN, -1, ENCODER_STEPS_PER_NOTCH) {
    initialized = false;
    buttonPressed = false;
    buttonPressTime = 0;
    lastButtonReleaseTime = 0;
    clickCount = 0;
    encoderValue = 0;
    encoderStep = 1;
    processingLongPress = false;
    processingDoubleClick = false;
}

bool UserInput::begin() {
    // 初始化编码器
    encoder.begin();
    encoder.setup(
        [] { g_encoder->readEncoder_ISR(); },
        [] { g_buttonChanged = true; }
    );
    
    // 存储全局指针，用于中断
    g_encoder = &encoder;
    
    // 设置编码器相关属性
    encoder.setAcceleration(250); // 加速度，数值越大响应越灵敏
    
    initialized = true;
    Serial.println("用户输入初始化成功");
    return true;
}

void UserInput::update() {
    if (!initialized) {
        return;
    }
    
    // 处理按钮状态变化
    if (g_buttonChanged) {
        g_buttonChanged = false;
        bool currentState = encoder.isEncoderButtonDown();
        
        // 按钮按下
        if (currentState && !buttonPressed) {
            buttonPressed = true;
            buttonPressTime = millis();
        }
        // 按钮释放
        else if (!currentState && buttonPressed) {
            buttonPressed = false;
            unsigned long pressDuration = millis() - buttonPressTime;
            
            // 检测长按
            if (pressDuration >= LONG_PRESS_TIME) {
                processingLongPress = true;
            } 
            // 检测单击/双击
            else {
                clickCount++;
                if (clickCount == 1) {
                    lastButtonReleaseTime = millis();
                } else if (clickCount == 2) {
                    processingDoubleClick = true;
                    clickCount = 0;
                }
            }
        }
    }
    
    // 检查双击超时
    if (clickCount == 1 && (millis() - lastButtonReleaseTime > DOUBLE_CLICK_TIME)) {
        clickCount = 0;
    }
    
    // 读取编码器值变化
    int16_t newValue = encoder.readEncoder() / ENCODER_STEPS_PER_NOTCH;
    if (newValue != encoderValue) {
        encoderValue = newValue;
    }
}

EncoderEvent UserInput::getEvent() {
    if (!initialized) {
        return EV_NONE;
    }
    
    // 检查长按事件
    if (processingLongPress) {
        processingLongPress = false;
        return EV_LONG_PRESS;
    }
    
    // 检查双击事件
    if (processingDoubleClick) {
        processingDoubleClick = false;
        return EV_DOUBLE_CLICK;
    }
    
    // 检查单击事件
    if (clickCount == 0 && (millis() - lastButtonReleaseTime <= DOUBLE_CLICK_TIME) && 
        lastButtonReleaseTime > 0) {
        lastButtonReleaseTime = 0;  // 重置以避免重复检测
        return EV_SINGLE_CLICK;
    }
    
    // 检查旋转事件
    static int16_t lastReadValue = encoderValue;
    if (encoderValue > lastReadValue) {
        lastReadValue = encoderValue;
        return EV_ROTATE_CW;
    } else if (encoderValue < lastReadValue) {
        lastReadValue = encoderValue;
        return EV_ROTATE_CCW;
    }
    
    return EV_NONE;
}

int16_t UserInput::getEncoderValue() {
    return encoderValue;
}

void UserInput::setEncoderValue(int16_t value) {
    encoderValue = value;
    encoder.setEncoderValue(value * ENCODER_STEPS_PER_NOTCH);
}

void UserInput::setEncoderStep(uint8_t step) {
    encoderStep = step;
}

uint8_t UserInput::getEncoderStep() {
    return encoderStep;
}

void IRAM_ATTR UserInput::handleButtonInterrupt() {
    g_buttonChanged = true;
}

void IRAM_ATTR UserInput::handleEncoderInterrupt() {
    if (g_encoder != nullptr) {
        g_encoder->readEncoder_ISR();
    }
} 