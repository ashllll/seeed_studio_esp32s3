#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "config.h"

// 显示页面
enum DisplayPage {
    PAGE_MAIN = 0,        // 主页面
    PAGE_MENU,            // 菜单页面
    PAGE_CALIBRATION,     // 校准页面
    PAGE_ERROR            // 错误页面
};

class DisplayManager {
private:
    Adafruit_SSD1306 display;
    bool initialized;
    DisplayPage currentPage;
    unsigned long lastRefreshTime;
    
    // 错误信息
    char errorMessage[32];
    ErrorCode errorCode;
    
    // 显示参数
    float currentTemp;
    float targetTemp;
    uint8_t powerPercentage;
    SystemState systemState;
    
    // 动画条参数
    uint8_t animationFrame;
    
    // 绘制不同页面
    void drawMainPage();
    void drawMenuPage();
    void drawCalibrationPage();
    void drawErrorPage();
    
    // 辅助绘制函数
    void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percentage);
    void drawAnimatedBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percentage);

public:
    DisplayManager();
    
    // 初始化显示器
    bool begin();
    
    // 更新显示
    void update();
    
    // 设置当前页面
    void setPage(DisplayPage page);
    
    // 获取当前页面
    DisplayPage getPage();
    
    // 设置温度
    void setTemperature(float current, float target);
    
    // 设置功率百分比
    void setPowerPercentage(uint8_t percentage);
    
    // 设置系统状态
    void setSystemState(SystemState state);
    
    // 显示错误信息
    void showError(ErrorCode code, const char* message);
    
    // 清除错误信息
    void clearError();
};

#endif // DISPLAY_MANAGER_H 