#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>
#include <PID_v1.h>
#include "config.h"

class PIDController {
private:
    double input;        // 当前温度
    double filteredInput; // 滤波后的温度输入
    double output;       // 控制输出 (PWM占空比)
    double setpoint;     // 设定温度
    
    // PID参数
    double kp, ki, kd;
    double deadband; // 死区参数，用于开关量控制
    
    // PID控制器
    PID* pid;
    
    // 最后一次计算时间
    unsigned long lastCompute;
    
    // 自整定相关
    bool isAutoTuning;      // 是否正在自整定
    unsigned long autoTuneStartTime; // 自整定开始时间
    double autoTuneOutput;  // 自整定输出值
    unsigned long autoTuneCycles; // 自整定周期计数
    double autoTunePeakMax; // 自整定最大峰值
    double autoTunePeakMin; // 自整定最小峰值
    unsigned long autoTuneLastPeakTime; // 上次峰值时间
    double autoTuneKu;      // 自整定极限增益
    double autoTuneTu;      // 自整定振荡周期
    
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
    void setTunings(double _kp, double _ki, double _kd, double _deadband = 0.5);
    
    // 获取PID参数
    void getTunings(double *_kp, double *_ki, double *_kd);
    
    // 自动调整PID参数，使用继电器反馈方法
    bool autoTune(double outputOn = 1023, double outputOff = 0, unsigned long testDuration = 600000);
    void startAutoTune();
    bool isParametersValid();
    void dynamicTuneOnStartup();
};

#endif // PID_CONTROLLER_H 