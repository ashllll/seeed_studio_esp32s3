#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>
#include <PID_v1.h>
#include "config.h"

class PIDController {
private:
    double input;        // 当前温度
    double output;       // 控制输出 (PWM占空比)
    double setpoint;     // 设定温度
    
    // PID参数
    double kp, ki, kd;
    
    // PID控制器
    PID* pid;
    
    // 最后一次计算时间
    unsigned long lastCompute;
    
public:
    PIDController();
    ~PIDController();
    
    // 初始化PID控制器
    bool begin();
    
    // 设置目标温度
    void setTargetTemp(double target);
    
    // 获取目标温度
    double getTargetTemp();
    
    // 设置当前温度
    void setCurrentTemp(double current);
    
    // 获取控制输出
    double getOutput();
    
    // 计算PID输出
    bool compute();
    
    // 设置PID参数
    void setTunings(double _kp, double _ki, double _kd);
    
    // 获取PID参数
    void getTunings(double *_kp, double *_ki, double *_kd);
    
    // 自动调整PID参数
    bool autoTune();
};

#endif // PID_CONTROLLER_H 