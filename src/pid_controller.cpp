#include "pid_controller.h"

PIDController::PIDController() {
    input = 0.0;
    output = 0.0;
    setpoint = TEMP_DEFAULT;
    
    kp = PID_KP_DEFAULT;
    ki = PID_KI_DEFAULT;
    kd = PID_KD_DEFAULT;
    
    pid = nullptr;
    lastCompute = 0;
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
    input = current;
}

double PIDController::getOutput() {
    return output;
}

bool PIDController::compute() {
    // 检查计算间隔
    unsigned long now = millis();
    if (now - lastCompute < PID_COMPUTE_INTERVAL && lastCompute > 0) {
        return false;
    }
    
    lastCompute = now;
    
    // 调用PID库的计算功能
    return pid->Compute();
}

void PIDController::setTunings(double _kp, double _ki, double _kd) {
    kp = _kp;
    ki = _ki;
    kd = _kd;
    
    if (pid != nullptr) {
        pid->SetTunings(kp, ki, kd);
    }
}

void PIDController::getTunings(double *_kp, double *_ki, double *_kd) {
    *_kp = kp;
    *_ki = ki;
    *_kd = kd;
}

bool PIDController::autoTune() {
    // 简化的自动调整实现
    // 注意：真实实现可能需要更复杂的算法，如继电器反馈方法
    
    // 这里只是一个示例框架，实际使用需要替换为合适的自整定算法
    Serial.println("开始PID自动调整");
    
    // 简化版本: 根据系统响应特性估算参数
    // 这里可以实现继电器反馈法或其他算法
    
    // 示例值 - 实际应用需要根据系统特性调整
    double newKp = PID_KP_DEFAULT * 1.1;
    double newKi = PID_KI_DEFAULT * 1.05;
    double newKd = PID_KD_DEFAULT * 0.95;
    
    // 设置新参数
    setTunings(newKp, newKi, newKd);
    
    Serial.println("PID自动调整完成");
    Serial.print("新参数: Kp=");
    Serial.print(newKp);
    Serial.print(", Ki=");
    Serial.print(newKi);
    Serial.print(", Kd=");
    Serial.println(newKd);
    
    return true;
} 