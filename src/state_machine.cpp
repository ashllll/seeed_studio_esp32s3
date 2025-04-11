#include "state_machine.h"
#include <EEPROM.h>

// 全局变量用于看门狗中断
static volatile bool g_watchdogTriggered = false;

StateMachine::StateMachine(
    TempSensor* _tempSensor,
    PIDController* _pidController,
    PWMController* _pwmController,
    DisplayManager* _displayManager,
    UserInput* _userInput
) {
    tempSensor = _tempSensor;
    pidController = _pidController;
    pwmController = _pwmController;
    displayManager = _displayManager;
    userInput = _userInput;
    
    currentState = STATE_IDLE;
    previousState = STATE_IDLE;
    currentError = ERROR_NONE;
    
    watchdogTimer = nullptr;
    watchdogEnabled = false;
    
    menuSelection = 0;
    calibrationOffset = 0.0f;
}

bool StateMachine::begin() {
    // 初始化EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // 加载设置
    loadSettings();
    
    // 初始化看门狗定时器
    watchdogTimer = timerBegin(0, 80, true); // 1MHz (80MHz / 80)
    timerAttachInterrupt(watchdogTimer, &watchdogInterrupt, true);
    timerAlarmWrite(watchdogTimer, WATCHDOG_TIMEOUT * 1000, false); // 转换为微秒
    timerAlarmEnable(watchdogTimer);
    watchdogEnabled = true;
    
    // 执行系统自检
    if (!performSelfTest()) {
        setError(ERROR_SYSTEM, "Self test failed");
        return false;
    }
    
    // 设置初始状态
    setState(STATE_IDLE);
    
    Serial.println("状态机初始化成功");
    return true;
}

void StateMachine::update() {
    // 重置看门狗
    resetWatchdog();
    
    // 检查是否触发了看门狗
    if (g_watchdogTriggered) {
        g_watchdogTriggered = false;
        setError(ERROR_SYSTEM, "Watchdog triggered");
        return;
    }
    
    // 安全检查 (适用于所有状态)
    ErrorCode safetyError = performSafetyChecks();
    if (safetyError != ERROR_NONE) {
        // 根据错误类型设置错误信息
        switch (safetyError) {
            case ERROR_TEMP_SENSOR:
                setError(safetyError, "Temperature sensor");
                break;
            case ERROR_OVERTEMP:
                setError(safetyError, "Over temperature");
                break;
            case ERROR_HEATER:
                setError(safetyError, "Heater fault");
                break;
            case ERROR_POWER:
                setError(safetyError, "Power fault");
                break;
            default:
                setError(safetyError, "Unknown error");
                break;
        }
        return;
    }
    
    // 根据当前状态处理
    switch (currentState) {
        case STATE_IDLE:
            handleIdleState();
            break;
        case STATE_WORKING:
            handleWorkingState();
            break;
        case STATE_CALIBRATION:
            handleCalibrationState();
            break;
        case STATE_MENU:
            handleMenuState();
            break;
        case STATE_ERROR:
            handleErrorState();
            break;
    }
}

void StateMachine::handleIdleState() {
    // 更新显示
    float currentTemp = tempSensor->readTemperature();
    float targetTemp = pidController->getTargetTemp();
    
    displayManager->setTemperature(currentTemp, targetTemp);
    displayManager->setPowerPercentage(0);
    displayManager->setSystemState(STATE_IDLE);
    displayManager->setPage(PAGE_MAIN);
    
    // 检查用户输入
    userInput->update();
    EncoderEvent event = userInput->getEvent();
    
    switch (event) {
        case EV_SINGLE_CLICK:
            // 单击启动加热
            setState(STATE_WORKING);
            break;
        
        case EV_LONG_PRESS:
            // 长按进入菜单
            setState(STATE_MENU);
            break;
            
        case EV_ROTATE_CW:
            // 顺时针旋转增加目标温度
            pidController->setTargetTemp(targetTemp + userInput->getEncoderStep());
            break;
            
        case EV_ROTATE_CCW:
            // 逆时针旋转减少目标温度
            pidController->setTargetTemp(targetTemp - userInput->getEncoderStep());
            break;
            
        default:
            break;
    }
}

void StateMachine::handleWorkingState() {
    // 读取当前温度
    float currentTemp = tempSensor->readTemperature();
    float targetTemp = pidController->getTargetTemp();
    
    // 设置PID输入
    pidController->setCurrentTemp(currentTemp);
    
    // 计算PID输出
    if (pidController->compute()) {
        // 更新PWM输出
        pwmController->setDutyCycle(pidController->getOutput());
    }
    
    // 更新显示
    displayManager->setTemperature(currentTemp, targetTemp);
    displayManager->setPowerPercentage(pwmController->getPowerPercentage());
    displayManager->setSystemState(STATE_WORKING);
    displayManager->setPage(PAGE_MAIN);
    
    // 检查用户输入
    userInput->update();
    EncoderEvent event = userInput->getEvent();
    
    switch (event) {
        case EV_DOUBLE_CLICK:
            // 双击停止加热
            pwmController->disable();
            setState(STATE_IDLE);
            break;
            
        case EV_LONG_PRESS:
            // 长按进入菜单 (先停止加热)
            pwmController->disable();
            setState(STATE_MENU);
            break;
            
        case EV_ROTATE_CW:
            // 顺时针旋转增加目标温度
            pidController->setTargetTemp(targetTemp + userInput->getEncoderStep());
            break;
            
        case EV_ROTATE_CCW:
            // 逆时针旋转减少目标温度
            pidController->setTargetTemp(targetTemp - userInput->getEncoderStep());
            break;
            
        default:
            break;
    }
}

void StateMachine::handleCalibrationState() {
    // 读取当前温度
    float currentTemp = tempSensor->readTemperature();
    float calibTemp = currentTemp + calibrationOffset;
    
    // 更新显示
    displayManager->setTemperature(currentTemp, calibTemp);
    displayManager->setSystemState(STATE_CALIBRATION);
    displayManager->setPage(PAGE_CALIBRATION);
    
    // 检查用户输入
    userInput->update();
    EncoderEvent event = userInput->getEvent();
    
    switch (event) {
        case EV_DOUBLE_CLICK:
            // 双击返回
            setState(previousState);
            break;
            
        case EV_LONG_PRESS:
            // 长按保存校准
            tempSensor->setCalibration(calibrationOffset);
            saveSettings();
            setState(previousState);
            break;
            
        case EV_ROTATE_CW:
            // 顺时针旋转增加校准值
            calibrationOffset += 0.1f;
            break;
            
        case EV_ROTATE_CCW:
            // 逆时针旋转减少校准值
            calibrationOffset -= 0.1f;
            break;
            
        default:
            break;
    }
}

void StateMachine::handleMenuState() {
    // 更新显示
    displayManager->setSystemState(STATE_MENU);
    displayManager->setPage(PAGE_MENU);
    
    // 检查用户输入
    userInput->update();
    EncoderEvent event = userInput->getEvent();
    
    switch (event) {
        case EV_SINGLE_CLICK:
            // 单击选择菜单项
            switch (menuSelection) {
                case 0: // PID参数
                    // 这里应该有PID参数调整逻辑
                    break;
                case 1: // 校准
                    previousState = currentState;
                    setState(STATE_CALIBRATION);
                    break;
                case 2: // 系统信息
                    // 这里应该有系统信息显示逻辑
                    break;
                case 3: // 重置默认值
                    // 重置默认值
                    pidController->setTunings(PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
                    tempSensor->setCalibration(0.0f);
                    saveSettings();
                    break;
            }
            break;
            
        case EV_DOUBLE_CLICK:
            // 双击返回
            setState(STATE_IDLE);
            break;
            
        case EV_ROTATE_CW:
            // 顺时针旋转选择下一个菜单项
            menuSelection = (menuSelection + 1) % 4;
            break;
            
        case EV_ROTATE_CCW:
            // 逆时针旋转选择上一个菜单项
            menuSelection = (menuSelection + 3) % 4; // +3 instead of -1 to avoid negative
            break;
            
        default:
            break;
    }
}

void StateMachine::handleErrorState() {
    // 确保加热器关闭
    pwmController->emergencyStop();
    
    // 更新显示
    displayManager->setSystemState(STATE_ERROR);
    displayManager->setPage(PAGE_ERROR);
    
    // 检查用户输入
    userInput->update();
    EncoderEvent event = userInput->getEvent();
    
    // 只响应长按重置
    if (event == EV_LONG_PRESS) {
        clearError();
        setState(STATE_IDLE);
    }
}

ErrorCode StateMachine::performSafetyChecks() {
    // 检查温度传感器
    if (!tempSensor->checkSensor()) {
        return ERROR_TEMP_SENSOR;
    }
    
    // 读取当前温度
    float currentTemp = tempSensor->readTemperature();
    
    // 检查过温
    if (currentTemp > TEMP_PROTECTION_MAX) {
        return ERROR_OVERTEMP;
    }
    
    // 检查加热器状态 (可以添加更复杂的逻辑)
    // ...
    
    // 检查电源状态 (可以添加更复杂的逻辑)
    // ...
    
    return ERROR_NONE;
}

bool StateMachine::performSelfTest() {
    // 简单的自检流程
    Serial.println("执行系统自检...");
    
    // 检查温度传感器
    if (!tempSensor->checkSensor()) {
        Serial.println("温度传感器自检失败");
        return false;
    }
    
    Serial.println("系统自检通过");
    return true;
}

void StateMachine::setError(ErrorCode error, const char* message) {
    currentError = error;
    
    // 安全措施 - 停止PWM输出
    pwmController->emergencyStop();
    
    // 设置错误显示
    displayManager->showError(error, message);
    
    // 切换到错误状态
    setState(STATE_ERROR);
    
    Serial.print("错误: ");
    Serial.print(error);
    Serial.print(" - ");
    Serial.println(message);
}

void StateMachine::clearError() {
    currentError = ERROR_NONE;
    displayManager->clearError();
    
    Serial.println("错误已清除");
}

void StateMachine::resetWatchdog() {
    if (watchdogEnabled && watchdogTimer != nullptr) {
        timerWrite(watchdogTimer, 0);
    }
}

void StateMachine::loadSettings() {
    Serial.println("从EEPROM加载设置...");
    
    // 读取PID参数
    float kp, ki, kd;
    EEPROM.get(EEPROM_PID_KP_ADDR, kp);
    EEPROM.get(EEPROM_PID_KI_ADDR, ki);
    EEPROM.get(EEPROM_PID_KD_ADDR, kd);
    
    // 检查PID参数是否有效 (避免NaN或过大/过小的值)
    if (isnan(kp) || isnan(ki) || isnan(kd) ||
        kp < 0.01f || kp > 1000.0f ||
        ki < 0.0f || ki > 1000.0f ||
        kd < 0.0f || kd > 1000.0f) {
        // 使用默认值
        kp = PID_KP_DEFAULT;
        ki = PID_KI_DEFAULT;
        kd = PID_KD_DEFAULT;
    }
    
    // 设置PID参数
    pidController->setTunings(kp, ki, kd);
    
    // 读取目标温度
    float targetTemp;
    EEPROM.get(EEPROM_TARGET_TEMP_ADDR, targetTemp);
    
    // 检查目标温度是否有效
    if (isnan(targetTemp) || targetTemp < TEMP_MIN || targetTemp > TEMP_MAX) {
        targetTemp = TEMP_DEFAULT;
    }
    
    // 设置目标温度
    pidController->setTargetTemp(targetTemp);
    
    // 读取温度校准值
    float tempCalibration;
    EEPROM.get(EEPROM_TEMP_CALIBRATION_ADDR, tempCalibration);
    
    // 检查校准值是否有效
    if (isnan(tempCalibration) || tempCalibration < -10.0f || tempCalibration > 10.0f) {
        tempCalibration = 0.0f;
    }
    
    // 设置温度校准值
    tempSensor->setCalibration(tempCalibration);
    calibrationOffset = tempCalibration;
    
    Serial.println("设置加载完成");
}

void StateMachine::saveSettings() {
    Serial.println("保存设置到EEPROM...");
    
    // 保存PID参数
    double kp, ki, kd;
    pidController->getTunings(&kp, &ki, &kd);
    
    EEPROM.put(EEPROM_PID_KP_ADDR, (float)kp);
    EEPROM.put(EEPROM_PID_KI_ADDR, (float)ki);
    EEPROM.put(EEPROM_PID_KD_ADDR, (float)kd);
    
    // 保存目标温度
    float targetTemp = pidController->getTargetTemp();
    EEPROM.put(EEPROM_TARGET_TEMP_ADDR, targetTemp);
    
    // 保存温度校准值
    float tempCalibration = tempSensor->getCalibration();
    EEPROM.put(EEPROM_TEMP_CALIBRATION_ADDR, tempCalibration);
    
    // 提交更改
    EEPROM.commit();
    
    Serial.println("设置保存完成");
}

SystemState StateMachine::getState() {
    return currentState;
}

void StateMachine::setState(SystemState newState) {
    if (newState != currentState) {
        previousState = currentState;
        currentState = newState;
        
        // 状态切换处理
        switch (newState) {
            case STATE_IDLE:
                pwmController->disable();
                break;
                
            case STATE_WORKING:
                pwmController->enable();
                break;
                
            default:
                break;
        }
        
        Serial.print("状态切换: ");
        Serial.print(previousState);
        Serial.print(" -> ");
        Serial.println(currentState);
    }
}

ErrorCode StateMachine::getError() {
    return currentError;
}

void IRAM_ATTR StateMachine::watchdogInterrupt() {
    g_watchdogTriggered = true;
} 