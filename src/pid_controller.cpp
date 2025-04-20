#include "pid_controller.h"

PIDController::PIDController() {
    input = 0.0;
    filteredInput = 0.0;
    output = 0.0;
    setpoint = TEMP_DEFAULT;
    
    kp = PID_KP_DEFAULT;
    ki = PID_KI_DEFAULT;
    kd = PID_KD_DEFAULT;
    deadband = 0.5; // 默认死区为0.5度以提高精度
    
    pid = nullptr;
    lastCompute = 0;
    isAutoTuning = false;
    autoTuneStartTime = 0;
    autoTuneOutput = 0.0;
    autoTuneCycles = 0;
    autoTunePeakMax = 0.0;
    autoTunePeakMin = 0.0;
    autoTuneLastPeakTime = 0;
    autoTuneKu = 0.0;
    autoTuneTu = 0.0;
}

PIDController::~PIDController() {
    if (pid != nullptr) {
        delete pid;
    }
}

bool PIDController::begin() {
    // 创建PID控制器
    // 参数: &输入, &输出, &设定点, Kp, Ki, Kd, 控制方向
    pid = new PID(&input, &output, &setpoint, kp, ki, kd, DIRECT);
    
    if (pid == nullptr) {
        Serial.println("PID控制器初始化失败");
        return false;
    }
    
    // 设置输出范围 (0-1023: 10位PWM分辨率)
    pid->SetOutputLimits(0, 1023);
    
    // 启用PID控制器
    pid->SetMode(AUTOMATIC);
    
    // 设置采样时间
    pid->SetSampleTime(PID_COMPUTE_INTERVAL);
    
    Serial.println("PID控制器初始化成功");
    return true;
}

void PIDController::setTargetTemp(double target) {
    // 限制目标温度范围
    if (target < TEMP_MIN) {
        target = TEMP_MIN;
    } else if (target > TEMP_MAX) {
        target = TEMP_MAX;
    }
    
    setpoint = target;
}

double PIDController::getTargetTemp() {
    return setpoint;
}

void PIDController::setCurrentTemp(double current) {
    // 简单的移动平均滤波器以提高抗干扰性
    const double alpha = 0.8; // 滤波系数，0-1之间，越大越平滑
    if (filteredInput == 0.0) {
        filteredInput = current; // 第一次设置直接使用当前值
    } else {
        filteredInput = alpha * filteredInput + (1.0 - alpha) * current;
    }
    input = filteredInput;
}

double PIDController::getOutput() {
    return output;
}

bool PIDController::compute() {
    if (isAutoTuning) return false; // 正在自整定，不计算
    
    // 检查计算间隔
    unsigned long now = millis();
    if (now - lastCompute < PID_COMPUTE_INTERVAL && lastCompute > 0) {
        return false;
    }
    
    lastCompute = now;
    
    // 调用PID库的计算功能
    bool result = pid->Compute();
    
    // 由于现在只支持开关量控制，添加死区逻辑
    if (output > 0) {
        // 如果输出大于0，检查是否在死区内
        if (setpoint - input < deadband) {
            output = 0; // 如果在死区内，关闭输出
        } else {
            output = 1023; // 否则全开
        }
    } else {
        output = 0; // 输出为0时保持关闭
    }
    
    // 动态微调（仅在误差较大时调整）
    double error = setpoint - input;
    if (fabs(error) > 0.5) {
        // 误差较大时微调参数
        if (error > 0) {
            kp += 0.01; // 增加比例增益以更快响应
            if (kp > 50.0) kp = 50.0;
        }
        ki += 0.001; // 缓慢增加积分增益以减少稳态误差
        if (ki > 5.0) ki = 5.0;
        pid->SetTunings(kp, ki, kd);
    }
    
    return result;
}

void PIDController::setTunings(double _kp, double _ki, double _kd, double _deadband) {
    kp = _kp;
    ki = _ki;
    kd = _kd;
    deadband = _deadband;
    
    if (pid != nullptr) {
        pid->SetTunings(kp, ki, kd);
    }
}

void PIDController::getTunings(double *_kp, double *_ki, double *_kd) {
    *_kp = kp;
    *_ki = ki;
    *_kd = kd;
}

bool PIDController::autoTune(double outputOn, double outputOff, unsigned long testDuration) {
    Serial.println("开始PID自动调整 - 继电器反馈方法");
    
    // 继电器反馈法参数
    const unsigned long cycleTimeout = testDuration; // 测试总时长（毫秒）
    autoTuneStartTime = millis();
    autoTuneLastPeakTime = autoTuneStartTime;
    autoTunePeakMax = input;
    autoTunePeakMin = input;
    autoTuneCycles = 0;
    autoTuneOutput = outputOn;
    autoTuneKu = 0.0;
    autoTuneTu = 0.0;
    bool outputState = true; // true表示开启
    
    // 自整定状态
    isAutoTuning = true;
    
    Serial.println("自动调整阶段1: 寻找振荡特性");
    
    // 阶段1: 寻找振荡特性
    while (millis() - autoTuneStartTime < cycleTimeout) {
        // 更新当前温度
        double currentTemp = input;
        
        // 更新最大和最小温度
        if (currentTemp > autoTunePeakMax) autoTunePeakMax = currentTemp;
        if (currentTemp < autoTunePeakMin) autoTunePeakMin = currentTemp;
        
        // 确定切换阈值（使用当前设定点，调整为更小的误差范围以提高精度）
        double thresholdHigh = setpoint + 0.5;
        double thresholdLow = setpoint - 0.5;
        
        // 根据温度与阈值的关系切换输出状态
        if (outputState && currentTemp > thresholdHigh) {
            // 从高到低切换
            outputState = false;
            autoTuneOutput = outputOff;
            output = outputOff;
        } else if (!outputState && currentTemp < thresholdLow) {
            // 从低到高切换
            outputState = true;
            autoTuneOutput = outputOn;
            output = outputOn;
            autoTuneCycles++; // 完成一个周期
            
            // 计算周期时间
            if (autoTuneCycles > 1) {
                autoTuneTu = (double)(millis() - autoTuneLastPeakTime) / 1000.0;
                autoTuneLastPeakTime = millis();
                
                // 计算增益
                double amplitude = (autoTunePeakMax - autoTunePeakMin) / 2.0;
                autoTuneKu = (4.0 * (outputOn - outputOff)) / (M_PI * amplitude);
                
                // 重置峰值
                autoTunePeakMax = currentTemp;
                autoTunePeakMin = currentTemp;
            }
        }
        
        // 短暂延时
        delay(10);
    }
    
    // 阶段2: 计算PID参数 (改进版，针对±0.5°C精度)
    if (autoTuneCycles >= 3) {
        // 使用改进的 Ziegler-Nichols 公式，增加积分项以减少稳态误差
        kp = 0.5 * autoTuneKu;  // 降低比例增益以减少过冲
        ki = 1.5 * autoTuneKu / autoTuneTu;  // 增加积分增益以提高稳态精度
        kd = 0.05 * autoTuneKu * autoTuneTu;  // 降低微分增益以减少噪声影响
        
        // 设置新参数
        setTunings(kp, ki, kd, 0.5);
        
        Serial.println("PID自动调整完成");
        Serial.print("振荡周期: ");
        Serial.print(autoTuneTu);
        Serial.println(" 秒");
        Serial.print("极限增益 Ku: ");
        Serial.println(autoTuneKu);
        Serial.print("新参数: Kp=");
        Serial.print(kp);
        Serial.print(", Ki=");
        Serial.print(ki);
        Serial.print(", Kd=");
        Serial.println(kd);
    } else {
        Serial.println("自动调整失败: 未检测到足够振荡周期");
        // 使用默认参数
        setTunings(PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT, 0.5);
    }
    
    // 恢复正常PID控制
    output = 0; // 初始设置为关闭
    isAutoTuning = false;
    
    return autoTuneCycles >= 3;
}

void PIDController::startAutoTune() {
    isAutoTuning = true;
    autoTune(1023, 0, 600000); // 启动自整定，测试10分钟
}

bool PIDController::isParametersValid() {
    // 检查参数是否在合理范围内
    return (kp > 0.0 && kp < 100.0) && (ki >= 0.0 && ki < 10.0) && (kd >= 0.0 && kd < 50.0);
}

void PIDController::dynamicTuneOnStartup() {
    // 在启动时进行快速动态调整，基于历史数据和当前环境
    if (isParametersValid()) {
        // 轻微调整参数以适应当前环境
        double error = setpoint - input;
        if (fabs(error) > 1.0) {
            // 如果误差较大，增加比例增益
            kp += 0.1;
            if (kp > 50.0) kp = 50.0;
        } else if (fabs(error) > 0.5) {
            // 如果误差中等，增加积分增益
            ki += 0.05;
            if (ki > 5.0) ki = 5.0;
        }
        // 设置调整后的参数
        setTunings(kp, ki, kd, 0.5);
        Serial.println("启动时动态调整PID参数完成");
        Serial.print("调整后参数: Kp=");
        Serial.print(kp);
        Serial.print(", Ki=");
        Serial.print(ki);
        Serial.print(", Kd=");
        Serial.println(kd);
    } else {
        // 如果参数无效，进行完整自整定
        Serial.println("PID参数无效，启动完整自整定");
        startAutoTune();
    }
}