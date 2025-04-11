#include "display_manager.h"

DisplayManager::DisplayManager() 
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {
    initialized = false;
    currentPage = PAGE_MAIN;
    lastRefreshTime = 0;
    animationFrame = 0;
    
    currentTemp = 0.0f;
    targetTemp = TEMP_DEFAULT;
    powerPercentage = 0;
    systemState = STATE_IDLE;
    
    errorCode = ERROR_NONE;
    strcpy(errorMessage, "");
}

bool DisplayManager::begin() {
    // 初始化OLED显示屏
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306初始化失败");
        return false;
    }
    
    // 初始设置
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    
    // 显示启动画面
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(SYSTEM_NAME);
    display.setTextSize(1);
    display.setCursor(0, 16);
    display.println("Version: " + String(SYSTEM_VERSION));
    display.setCursor(0, 32);
    display.println("Initializing...");
    display.setCursor(0, 48);
    display.println("Copyright 2023");
    display.display();
    
    // 等待2秒
    delay(2000);
    
    initialized = true;
    Serial.println("显示管理器初始化成功");
    return true;
}

void DisplayManager::update() {
    if (!initialized) {
        return;
    }
    
    // 限制刷新率
    unsigned long now = millis();
    if (now - lastRefreshTime < UI_REFRESH_INTERVAL) {
        return;
    }
    lastRefreshTime = now;
    
    // 清屏
    display.clearDisplay();
    
    // 更新动画帧
    animationFrame = (animationFrame + 1) % 8;
    
    // 根据当前页面绘制
    switch (currentPage) {
        case PAGE_MAIN:
            drawMainPage();
            break;
        case PAGE_MENU:
            drawMenuPage();
            break;
        case PAGE_CALIBRATION:
            drawCalibrationPage();
            break;
        case PAGE_ERROR:
            drawErrorPage();
            break;
    }
    
    // 显示
    display.display();
}

void DisplayManager::drawMainPage() {
    // 左侧：目标温度和功率百分比
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("[");
    display.print((int)targetTemp);
    display.print("] SET");
    
    // 功率百分比和动画条
    display.setCursor(0, 30);
    display.print(powerPercentage);
    display.print("% ");
    drawAnimatedBar(20, 30, 40, 10, powerPercentage);
    
    // 右侧：当前温度 (大字号)
    display.setTextSize(2);
    display.setCursor(70, 15);
    display.print((int)currentTemp);
    display.setTextSize(1);
    display.print("C");
    
    // 系统状态
    display.setTextSize(1);
    display.setCursor(0, 54);
    
    switch (systemState) {
        case STATE_IDLE:
            display.print("IDLE");
            break;
        case STATE_WORKING:
            display.print("HEATING");
            break;
        case STATE_CALIBRATION:
            display.print("CALIBRATING");
            break;
        case STATE_MENU:
            display.print("MENU");
            break;
        case STATE_ERROR:
            display.print("ERROR!");
            break;
    }
}

void DisplayManager::drawMenuPage() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("SETTINGS MENU");
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    
    // 菜单项示例
    display.setCursor(0, 15);
    display.println("1. PID Parameters");
    display.setCursor(0, 25);
    display.println("2. Calibration");
    display.setCursor(0, 35);
    display.println("3. System Info");
    display.setCursor(0, 45);
    display.println("4. Reset Defaults");
    
    // 菜单导航提示
    display.setCursor(0, 55);
    display.println("Rotate:Select  Click:Enter");
}

void DisplayManager::drawCalibrationPage() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("TEMPERATURE CALIBRATION");
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    
    // 当前读数和校准目标
    display.setCursor(0, 15);
    display.print("Current: ");
    display.print(currentTemp, 1);
    display.println("C");
    
    display.setCursor(0, 25);
    display.print("Set Real: ");
    display.print(targetTemp, 1);
    display.println("C");
    
    // 偏差
    float offset = targetTemp - currentTemp;
    display.setCursor(0, 35);
    display.print("Offset: ");
    display.print(offset, 1);
    display.println("C");
    
    // 操作提示
    display.setCursor(0, 55);
    display.println("Rotate:Adjust  Long:Save");
}

void DisplayManager::drawErrorPage() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ERROR DETECTED!");
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    
    // 错误代码和消息
    display.setCursor(0, 15);
    display.print("Code: E");
    display.println(errorCode);
    
    display.setCursor(0, 25);
    display.println(errorMessage);
    
    // 安全状态提示
    display.setCursor(0, 45);
    display.println("Heater: DISABLED");
    
    // 操作提示
    display.setCursor(0, 55);
    display.println("Long press to reset");
}

void DisplayManager::drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percentage) {
    // 限制百分比范围
    if (percentage > 100) {
        percentage = 100;
    }
    
    // 绘制外框
    display.drawRect(x, y, width, height, SSD1306_WHITE);
    
    // 计算填充宽度
    uint16_t fillWidth = (percentage * (width - 2)) / 100;
    
    // 填充进度条
    if (fillWidth > 0) {
        display.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_WHITE);
    }
}

void DisplayManager::drawAnimatedBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percentage) {
    // 先绘制基本进度条
    drawProgressBar(x, y, width, height, percentage);
    
    // 添加动画效果 (如果功率大于0)
    if (percentage > 0) {
        // 计算动画位置
        uint16_t fillWidth = (percentage * (width - 2)) / 100;
        uint16_t animPos = (animationFrame * fillWidth) / 8;
        
        // 绘制动画点
        if (fillWidth > 2) {
            display.drawPixel(x + 1 + animPos, y + height/2, !SSD1306_WHITE);
        }
    }
}

void DisplayManager::setPage(DisplayPage page) {
    currentPage = page;
}

DisplayPage DisplayManager::getPage() {
    return currentPage;
}

void DisplayManager::setTemperature(float current, float target) {
    currentTemp = current;
    targetTemp = target;
}

void DisplayManager::setPowerPercentage(uint8_t percentage) {
    powerPercentage = percentage;
}

void DisplayManager::setSystemState(SystemState state) {
    systemState = state;
}

void DisplayManager::showError(ErrorCode code, const char* message) {
    errorCode = code;
    strncpy(errorMessage, message, sizeof(errorMessage) - 1);
    errorMessage[sizeof(errorMessage) - 1] = '\0'; // 确保字符串结束
    
    // 自动切换到错误页面
    setPage(PAGE_ERROR);
}

void DisplayManager::clearError() {
    errorCode = ERROR_NONE;
    strcpy(errorMessage, "");
} 