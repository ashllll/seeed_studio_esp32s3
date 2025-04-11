#ifndef UI_ADAPTER_H
#define UI_ADAPTER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "config.h"
#include "temp_sensor.h"
#include "pid_controller.h"
#include "pwm_controller.h"
#include "user_input.h"

// UI页面定义
enum UIPage {
    UI_PAGE_MAIN = 0,        // 主页面
    UI_PAGE_MENU,            // 主菜单
    UI_PAGE_PID_MENU,        // PID参数菜单
    UI_PAGE_CALIBRATION,     // 校准菜单
    UI_PAGE_SYSTEM_INFO,     // 系统信息页面
    UI_PAGE_ERROR            // 错误页面
};

// 菜单项类型
enum MenuItemType {
    ITEM_NORMAL = 0,     // 普通菜单项
    ITEM_SWITCH,         // 开关菜单项
    ITEM_SLIDER,         // 滑块菜单项
    ITEM_SUBMENU         // 子菜单项
};

// 菜单项定义
struct MenuItem {
    char title[20];          // 菜单项标题
    MenuItemType type;       // 菜单项类型
    UIPage targetPage;       // 目标页面（仅适用于子菜单）
    float* valuePtr;         // 值指针（用于滑块和开关）
    float minValue;          // 最小值（用于滑块）
    float maxValue;          // 最大值（用于滑块）
    float stepValue;         // 步长（用于滑块）
};

// UI适配器类
class UIAdapter {
private:
    Adafruit_SSD1306* display;  // 显示屏指针
    TempSensor* tempSensor;     // 温度传感器指针
    PIDController* pidController; // PID控制器指针
    PWMController* pwmController; // PWM控制器指针
    UserInput* userInput;       // 用户输入指针
    
    // UI状态
    UIPage currentPage;         // 当前页面
    UIPage previousPage;        // 上一页面
    uint8_t menuSelection;      // 菜单选择项
    bool valueEditing;          // 是否在编辑值
    bool initialized;           // 是否已初始化
    
    // 错误信息
    char errorMessage[32];      // 错误消息
    ErrorCode errorCode;        // 错误代码
    
    // 系统状态
    float currentTemp;          // 当前温度
    float targetTemp;           // 目标温度
    uint8_t powerPercentage;    // 功率百分比
    SystemState systemState;    // 系统状态
    
    // 动画参数
    uint8_t animationFrame;     // 动画帧
    unsigned long lastRefreshTime; // 上次刷新时间
    
    // 菜单参数
    static const uint8_t MAX_MENU_ITEMS = 6;            // 最大菜单项数
    MenuItem mainMenuItems[MAX_MENU_ITEMS];             // 主菜单项
    MenuItem pidMenuItems[MAX_MENU_ITEMS];              // PID菜单项
    MenuItem calibrationMenuItems[MAX_MENU_ITEMS];      // 校准菜单项
    uint8_t mainMenuItemCount;                          // 主菜单项数量
    uint8_t pidMenuItemCount;                           // PID菜单项数量
    uint8_t calibrationMenuItemCount;                   // 校准菜单项数量
    
    // 绘制不同页面
    void drawMainPage();
    void drawMainMenu();
    void drawPIDMenu();
    void drawCalibrationPage();
    void drawSystemInfoPage();
    void drawErrorPage();
    
    // 通用菜单绘制函数
    void drawMenu(MenuItem* items, uint8_t itemCount, const char* title);
    
    // 辅助绘制函数
    void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percentage);
    void drawAnimatedBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percentage);
    void drawSelector(uint16_t y, uint16_t width);
    
    // 初始化菜单项
    void initMenuItems();

public:
    UIAdapter(Adafruit_SSD1306* _display, TempSensor* _tempSensor, 
              PIDController* _pidController, PWMController* _pwmController,
              UserInput* _userInput);
    
    // 初始化UI
    bool begin();
    
    // 更新UI显示
    void update();
    
    // 处理用户输入
    void handleInput();
    
    // 设置当前页面
    void setPage(UIPage page);
    
    // 获取当前页面
    UIPage getPage();
    
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
    
    // 获取目标温度（通过UI修改后）
    float getTargetTemp();
};

#endif // UI_ADAPTER_H 