#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include "config.h"
#include "temp_sensor.h"
#include "pid_controller.h"
#include "pwm_controller.h"
#include "display_manager.h"
#include "user_input.h"

class StateMachine {
private:
    SystemState currentState;
    SystemState previousState;
    ErrorCode currentError;
    
    // 各模块的指针
    TempSensor* tempSensor;
    PIDController* pidController;
    PWMController* pwmController;
    DisplayManager* displayManager;
    UserInput* userInput;
    
    // 看门狗定时器相关
    hw_timer_t* watchdogTimer;
    bool watchdogEnabled;
    
    // 菜单状态相关
    uint8_t menuSelection;
    
    // 校准状态相关
    float calibrationOffset;
    
    // 状态处理函数
    void handleIdleState();
    void handleWorkingState();
    void handleCalibrationState();
    void handleMenuState();
    void handleErrorState();
    
    // 安全检查
    ErrorCode performSafetyChecks();
    
    // 系统自检
    bool performSelfTest();
    
    // 设置错误状态
    void setError(ErrorCode error, const char* message);
    
    // 清除错误状态
    void clearError();
    
    // 看门狗重置
    void resetWatchdog();
    
    // 从EEPROM加载设置
    void loadSettings();
    
    // 保存设置到EEPROM
    void saveSettings();

public:
    StateMachine(
        TempSensor* _tempSensor,
        PIDController* _pidController,
        PWMController* _pwmController,
        DisplayManager* _displayManager,
        UserInput* _userInput
    );
    
    // 初始化状态机
    bool begin();
    
    // 更新状态机
    void update();
    
    // 获取当前状态
    SystemState getState();
    
    // 设置状态
    void setState(SystemState newState);
    
    // 获取当前错误
    ErrorCode getError();
    
    // 看门狗中断处理函数
    static void IRAM_ATTR watchdogInterrupt();
};

#endif // STATE_MACHINE_H 