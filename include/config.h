#ifndef CONFIG_H
#define CONFIG_H

// 系统参数
#define SYSTEM_VERSION "1.0.0"
#define SYSTEM_NAME "ESP32-S3 温控系统"

// 硬件引脚定义
// I2C
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 6

// ADS1115
#define ADS1115_ADDR 0x48
#define NTC_CHANNEL 0

// OLED
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// 编码器
#define ENCODER_PIN_A 3
#define ENCODER_PIN_B 4
#define ENCODER_BTN_PIN 7
#define ENCODER_STEPS_PER_NOTCH 4

// PWM输出
#define PWM_PIN 8
#define PWM_CHANNEL 0
#define PWM_FREQ 20000  // 20kHz
#define PWM_RESOLUTION 10 // 10bit分辨率，0-1023

// 温度控制参数
#define TEMP_MIN 0.0f
#define TEMP_MAX 100.0f
#define TEMP_DEFAULT 25.0f

// NTC参数
#define NTC_R25 100000.0f  // 25℃时的电阻值 (100K)
#define NTC_B 3950.0f      // B值
#define NTC_SERIES_R 100000.0f // 串联电阻 (100K)
#define NTC_VCC 3.3f       // 参考电压

// PID参数默认值
#define PID_KP_DEFAULT 10.0f
#define PID_KI_DEFAULT 0.1f
#define PID_KD_DEFAULT 1.0f

// 安全保护参数
#define TEMP_PROTECTION_MAX 100.0f
#define TEMP_PROTECTION_MIN 0.0f
#define WATCHDOG_TIMEOUT 3000 // 3秒

// UI刷新间隔
#define UI_REFRESH_INTERVAL 100 // 毫秒
#define TEMP_SAMPLE_INTERVAL 100 // 毫秒
#define PID_COMPUTE_INTERVAL 100 // 毫秒

// EEPROM参数
#define EEPROM_SIZE 512
#define EEPROM_PID_KP_ADDR 0
#define EEPROM_PID_KI_ADDR 4
#define EEPROM_PID_KD_ADDR 8
#define EEPROM_TARGET_TEMP_ADDR 12
#define EEPROM_TEMP_CALIBRATION_ADDR 16

// 错误代码
enum ErrorCode {
    ERROR_NONE = 0,
    ERROR_TEMP_SENSOR = 1,   // 温度传感器错误
    ERROR_OVERTEMP = 2,      // 过温错误
    ERROR_HEATER = 3,        // 加热器错误
    ERROR_POWER = 4,         // 电源错误
    ERROR_SYSTEM = 5         // 系统错误
};

// 系统状态
enum SystemState {
    STATE_IDLE = 0,          // 待机状态
    STATE_WORKING = 1,       // 工作状态
    STATE_CALIBRATION = 2,   // 校准状态
    STATE_MENU = 3,          // 菜单状态
    STATE_ERROR = 4          // 错误状态
};

#endif // CONFIG_H 