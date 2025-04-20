#include "ui_adapter.h"

UIAdapter::UIAdapter(U8G2_SSD1306_128X64_NONAME_F_HW_I2C* _display, TempSensor* _tempSensor,
                    PIDController* _pidController, PWMController* _pwmController,
                    UserInput* _userInput) {
    display = _display;
    tempSensor = _tempSensor;
    pidController = _pidController;
    pwmController = _pwmController;
    userInput = _userInput;
    
    // 初始化状态
    currentPage = UI_PAGE_MAIN;
    previousPage = UI_PAGE_MAIN;
    menuSelection = 0;
    valueEditing = false;
    initialized = false;
    
    // 初始化动画参数
    animationFrame = 0;
    lastRefreshTime = 0;
    
    // 初始化错误信息
    errorCode = ERROR_NONE;
    strcpy(errorMessage, "");
    
    // 初始化系统状态
    currentTemp = 0.0f;
    targetTemp = TEMP_DEFAULT;
    powerPercentage = 0;
    systemState = STATE_IDLE;
    inUserItem = false;
    
    // 初始化菜单项计数
    mainMenuItemCount = 0;
    pidMenuItemCount = 0;
    calibrationMenuItemCount = 0;
}

bool UIAdapter::begin() {
    if (display == nullptr) {
        Serial.println("UI适配器初始化失败：显示器为空");
        return false;
    }
    
    // 初始化菜单项
    initMenuItems();
    
    initialized = true;
    Serial.println("UI适配器初始化成功");
    return true;
}

void UIAdapter::update() {
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
    display->clearBuffer();
    
    // 更新动画帧
    animationFrame = (animationFrame + 1) % 8;
    
    // 根据当前页面绘制
    switch (currentPage) {
        case UI_PAGE_MAIN:
            drawMainPage();
            break;
        case UI_PAGE_MENU:
            drawMainMenu();
            break;
        case UI_PAGE_PID_MENU:
            drawPIDMenu();
            break;
        case UI_PAGE_CALIBRATION:
            drawCalibrationPage();
            break;
        case UI_PAGE_SYSTEM_INFO:
            drawSystemInfoPage();
            break;
        case UI_PAGE_ERROR:
            drawErrorPage();
            break;
    }
    
    // 显示
    display->sendBuffer();
}

void UIAdapter::handleInput() {
    // 获取用户输入事件
    userInput->update();
    EncoderEvent event = userInput->getEvent();
    
    // 根据当前页面和事件处理
    switch (currentPage) {
        case UI_PAGE_MAIN:
            // 主页面输入处理
            switch (event) {
                case EV_SINGLE_CLICK:
                    // 在主页面单击进入菜单
                    setPage(UI_PAGE_MENU);
                    break;
                    
                case EV_ROTATE_CW:
                    // 顺时针旋转增加目标温度
                    targetTemp += 1.0f;
                    if (targetTemp > TEMP_MAX) targetTemp = TEMP_MAX;
                    break;
                    
                case EV_ROTATE_CCW:
                    // 逆时针旋转减少目标温度
                    targetTemp -= 1.0f;
                    if (targetTemp < TEMP_MIN) targetTemp = TEMP_MIN;
                    break;
                    
                default:
                    break;
            }
            break;
            
        case UI_PAGE_MENU:
        case UI_PAGE_PID_MENU:
            // 菜单页面通用处理
            MenuItem* currentMenuItems;
            uint8_t itemCount;
            
            if (currentPage == UI_PAGE_MENU) {
                currentMenuItems = mainMenuItems;
                itemCount = mainMenuItemCount;
            } else {
                currentMenuItems = pidMenuItems;
                itemCount = pidMenuItemCount;
            }
            
            if (valueEditing) {
                // 正在编辑值
                switch (event) {
                    case EV_SINGLE_CLICK:
                        // 单击完成编辑
                        valueEditing = false;
                        break;
                        
                    case EV_ROTATE_CW:
                        // 增加值
                        if (currentMenuItems[menuSelection].type == ITEM_SLIDER) {
                            *currentMenuItems[menuSelection].valuePtr += currentMenuItems[menuSelection].stepValue;
                            if (*currentMenuItems[menuSelection].valuePtr > currentMenuItems[menuSelection].maxValue) {
                                *currentMenuItems[menuSelection].valuePtr = currentMenuItems[menuSelection].maxValue;
                            }
                        }
                        break;
                        
                    case EV_ROTATE_CCW:
                        // 减少值
                        if (currentMenuItems[menuSelection].type == ITEM_SLIDER) {
                            *currentMenuItems[menuSelection].valuePtr -= currentMenuItems[menuSelection].stepValue;
                            if (*currentMenuItems[menuSelection].valuePtr < currentMenuItems[menuSelection].minValue) {
                                *currentMenuItems[menuSelection].valuePtr = currentMenuItems[menuSelection].minValue;
                            }
                        }
                        break;
                        
                    default:
                        break;
                }
            } else {
                // 正常菜单导航
                switch (event) {
                    case EV_SINGLE_CLICK:
                        // 单击选择菜单项
                        switch (currentMenuItems[menuSelection].type) {
                            case ITEM_NORMAL:
                                // 普通菜单项不做任何操作
                                break;
                                
                            case ITEM_SWITCH:
                                // 切换开关值
                                if (currentMenuItems[menuSelection].valuePtr != nullptr) {
                                    *currentMenuItems[menuSelection].valuePtr = !(*currentMenuItems[menuSelection].valuePtr);
                                }
                                break;
                                
                            case ITEM_SLIDER:
                                // 进入值编辑模式
                                valueEditing = true;
                                break;
                                
                            case ITEM_SUBMENU:
                                // 进入子菜单
                                setPage(currentMenuItems[menuSelection].targetPage);
                                menuSelection = 0; // 重置菜单选择
                                break;
                        }
                        break;
                        
                    case EV_DOUBLE_CLICK:
                        // 双击返回上一级
                        setPage(previousPage);
                        break;
                        
                    case EV_ROTATE_CW:
                        // 向下移动菜单选择
                        menuSelection = (menuSelection + 1) % itemCount;
                        break;
                        
                    case EV_ROTATE_CCW:
                        // 向上移动菜单选择
                        if (menuSelection == 0)
                            menuSelection = itemCount - 1;
                        else
                            menuSelection--;
                        break;
                        
                    default:
                        break;
                }
            }
            break;
            
        case UI_PAGE_CALIBRATION:
            // 校准页面输入处理
            switch (event) {
                case EV_SINGLE_CLICK:
                    // 单击完成校准
                    setPage(previousPage);
                    break;
                    
                case EV_ROTATE_CW:
                    // 校准页面值处理
                    if (menuSelection < calibrationMenuItemCount) {
                        *calibrationMenuItems[menuSelection].valuePtr += calibrationMenuItems[menuSelection].stepValue;
                        if (*calibrationMenuItems[menuSelection].valuePtr > calibrationMenuItems[menuSelection].maxValue) {
                            *calibrationMenuItems[menuSelection].valuePtr = calibrationMenuItems[menuSelection].maxValue;
                        }
                    }
                    break;
                    
                case EV_ROTATE_CCW:
                    // 校准页面值处理
                    if (menuSelection < calibrationMenuItemCount) {
                        *calibrationMenuItems[menuSelection].valuePtr -= calibrationMenuItems[menuSelection].stepValue;
                        if (*calibrationMenuItems[menuSelection].valuePtr < calibrationMenuItems[menuSelection].minValue) {
                            *calibrationMenuItems[menuSelection].valuePtr = calibrationMenuItems[menuSelection].minValue;
                        }
                    }
                    break;
                    
                default:
                    break;
            }
            break;
            
        case UI_PAGE_SYSTEM_INFO:
            // 系统信息页面输入处理
            if (event == EV_SINGLE_CLICK || event == EV_DOUBLE_CLICK) {
                // 点击返回
                setPage(previousPage);
            }
            break;
            
        case UI_PAGE_ERROR:
            // 错误页面输入处理
            if (event == EV_LONG_PRESS) {
                // 长按清除错误
                clearError();
                setPage(UI_PAGE_MAIN);
            }
            break;
    }
}

void UIAdapter::drawMainPage() {
    // 左侧：目标温度和功率百分比
    display->setFont(u8g2_font_ncenB08_tr);
    display->drawStr(0, 10, ("[" + String((int)targetTemp) + "] SET").c_str());
    
    // 功率百分比和动画条
    display->drawStr(0, 40, (String(powerPercentage) + "%").c_str());
    drawAnimatedBar(20, 35, 40, 10, powerPercentage);
    
    // 右侧：当前温度 (大字号)
    display->setFont(u8g2_font_ncenB14_tr);
    display->drawStr(70, 30, (String((int)currentTemp) + "C").c_str());
    
    // 系统状态
    display->setFont(u8g2_font_ncenB08_tr);
    // 临时解决方案，之后需要实现 getSystemStateString()
    String stateStr;
    switch (systemState) {
        case STATE_IDLE: stateStr = "IDLE"; break;
        case STATE_WORKING: stateStr = "WORKING"; break;
        case STATE_CALIBRATION: stateStr = "CALIBRATION"; break;
        case STATE_MENU: stateStr = "MENU"; break;
        case STATE_ERROR: stateStr = "ERROR"; break;
        default: stateStr = "UNKNOWN";
    }
    display->drawStr(0, 60, stateStr.c_str());
    
    // 操作提示
    display->drawStr(70, 60, "Click:MENU");
}

void UIAdapter::drawMainMenu() {
    drawMenu(mainMenuItems, mainMenuItemCount, "MAIN MENU");
}

void UIAdapter::drawPIDMenu() {
    drawMenu(pidMenuItems, pidMenuItemCount, "PID PARAMETERS");
}

void UIAdapter::drawCalibrationPage() {
    // 卡片式布局 - 校准卡片
    display->setDrawColor(1);
    display->drawBox(5, 5, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10);
    display->setDrawColor(0);
    display->drawBox(7, 7, SCREEN_WIDTH - 14, SCREEN_HEIGHT - 14);
    display->setDrawColor(1);
    
    // 卡片标题
    display->setFont(u8g2_font_ncenB08_tr);
    display->drawStr(10, 15, "Calibration");
    
    // 当前读数和校准目标
    display->drawStr(10, 30, ("Current: " + String(currentTemp, 1) + "C").c_str());
    display->drawStr(10, 40, ("Set Real: " + String(targetTemp, 1) + "C").c_str());
    
    // 偏差
    float offset = targetTemp - currentTemp;
    display->drawStr(10, 50, ("Offset: " + String(offset, 1) + "C").c_str());
    
    // 操作提示
    display->drawStr(10, 60, "Rotate:Adjust Click:Save");
}

void UIAdapter::drawSystemInfoPage() {
    // 卡片式布局 - 系统信息卡片
    display->setDrawColor(1);
    display->drawBox(5, 5, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10);
    display->setDrawColor(0);
    display->drawBox(7, 7, SCREEN_WIDTH - 14, SCREEN_HEIGHT - 14);
    display->setDrawColor(1);
    
    // 卡片标题
    display->setFont(u8g2_font_ncenB08_tr);
    display->drawStr(10, 15, "System Info");
    
    // 版本信息
    display->drawStr(10, 30, ("Version: " + String(SYSTEM_VERSION)).c_str());
    
    // 系统运行时间
    unsigned long runTime = millis() / 1000; // 秒
    display->drawStr(10, 40, ("Uptime: " + String(runTime / 3600) + "h " + String((runTime % 3600) / 60) + "m").c_str());
    
    // PID参数
    double kp, ki, kd;
    pidController->getTunings(&kp, &ki, &kd);
    display->drawStr(10, 50, ("PID: " + String(kp, 1) + "/" + String(ki, 1) + "/" + String(kd, 1)).c_str());
    
    // 操作提示
    display->drawStr(10, 60, "Click: Return");
}

void UIAdapter::drawErrorPage() {
    // 卡片式布局 - 错误卡片
    display->setDrawColor(1);
    display->drawBox(5, 5, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10);
    display->setDrawColor(0);
    display->drawBox(7, 7, SCREEN_WIDTH - 14, SCREEN_HEIGHT - 14);
    display->setDrawColor(1);
    
    // 卡片标题
    display->setFont(u8g2_font_ncenB08_tr);
    display->drawStr(10, 15, "Error Detected!");
    
    // 错误代码和消息
    display->drawStr(10, 30, ("Code: E" + String(errorCode)).c_str());
    display->drawStr(10, 40, errorMessage);
    
    // 安全状态提示
    display->drawStr(10, 50, "Heater: DISABLED");
    
    // 操作提示
    display->drawStr(10, 60, "Long press: Reset");
}

void UIAdapter::drawMenu(MenuItem* items, uint8_t itemCount, const char* title) {
    // 标题
    display->setFont(u8g2_font_ncenB08_tr);
    display->drawStr(0, 10, title);
    display->drawHLine(0, 12, SCREEN_WIDTH);
    
    // 确定显示的菜单项范围 (最多显示4项)
    int startIdx = 0;
    if (menuSelection > 1 && itemCount > 4) {
        startIdx = menuSelection - 1;
    }
    if (startIdx + 4 > itemCount) {
        startIdx = itemCount - 4;
    }
    if (startIdx < 0) {
        startIdx = 0;
    }
    
    // 绘制菜单项
    for (int i = 0; i < 4 && i + startIdx < itemCount; i++) {
        int idx = i + startIdx;
        int y = 25 + i * 10;
        
        // 绘制选择器
        if (idx == menuSelection) {
            if (valueEditing) {
                // 编辑模式下使用不同样式
                display->drawBox(0, y - 1, 3, 9);
            } else {
                // 选中项
                drawSelector(y, 120);
            }
        }
        
        // 绘制菜单项文本
        display->drawStr(5, y, items[idx].title);
        
        // 根据类型绘制额外内容
        switch (items[idx].type) {
            case ITEM_SWITCH:
                // 开关项
                if (items[idx].valuePtr != nullptr) {
                    display->drawStr(100, y, *items[idx].valuePtr > 0 ? "ON" : "OFF");
                }
                break;
                
            case ITEM_SLIDER:
                // 滑块项
                if (items[idx].valuePtr != nullptr) {
                    display->drawStr(100, y, String(*items[idx].valuePtr, 1).c_str());
                }
                break;
                
            case ITEM_SUBMENU:
                // 子菜单项
                display->drawStr(110, y, ">");
                break;
                
            default:
                break;
        }
    }
    
    // 操作提示
    display->drawStr(0, 60, valueEditing ? "Rotate:Adjust Click:Save" : "Rotate:Move Click:Select");
}

void UIAdapter::drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percentage) {
    // 限制百分比范围
    if (percentage > 100) {
        percentage = 100;
    }
    
    // 绘制外框
    display->drawFrame(x, y, width, height);
    
    // 计算填充宽度
    uint16_t fillWidth = (percentage * (width - 2)) / 100;
    
    // 填充进度条
    if (fillWidth > 0) {
        display->drawBox(x + 1, y + 1, fillWidth, height - 2);
    }
}

void UIAdapter::drawAnimatedBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t percentage) {
    // 先绘制基本进度条
    drawProgressBar(x, y, width, height, percentage);
    
    // 添加动画效果 (如果功率大于0)
    if (percentage > 0) {
        // 计算动画位置
        uint16_t fillWidth = (percentage * (width - 2)) / 100;
        uint16_t animPos = (animationFrame * fillWidth) / 8;
        
        // 绘制动画点
        if (fillWidth > 2) {
            display->drawPixel(x + 1 + animPos, y + height/2);
        }
    }
}

void UIAdapter::drawSelector(uint16_t y, uint16_t width) {
    // 绘制选择器框
    display->drawFrame(0, y - 2, width, 11);
    
    // 左侧填充
    display->drawBox(0, y - 1, 3, 9);
}

void UIAdapter::initMenuItems() {
    // 初始化主菜单项
    mainMenuItemCount = 0;
    
    // 添加"PID参数"菜单项
    strcpy(mainMenuItems[mainMenuItemCount].title, "PID Parameters");
    mainMenuItems[mainMenuItemCount].type = ITEM_SUBMENU;
    mainMenuItems[mainMenuItemCount].targetPage = UI_PAGE_PID_MENU;
    mainMenuItemCount++;
    
    // 添加"校准"菜单项
    strcpy(mainMenuItems[mainMenuItemCount].title, "Calibration");
    mainMenuItems[mainMenuItemCount].type = ITEM_SUBMENU;
    mainMenuItems[mainMenuItemCount].targetPage = UI_PAGE_CALIBRATION;
    mainMenuItemCount++;
    
    // 添加"系统信息"菜单项
    strcpy(mainMenuItems[mainMenuItemCount].title, "System Info");
    mainMenuItems[mainMenuItemCount].type = ITEM_SUBMENU;
    mainMenuItems[mainMenuItemCount].targetPage = UI_PAGE_SYSTEM_INFO;
    mainMenuItemCount++;
    
    // 添加"加热控制"菜单项（开关类型）
    strcpy(mainMenuItems[mainMenuItemCount].title, "Heating");
    mainMenuItems[mainMenuItemCount].type = ITEM_SWITCH;
    static float heatingEnabled = 0.0f; // 默认关闭
    mainMenuItems[mainMenuItemCount].valuePtr = &heatingEnabled;
    mainMenuItemCount++;
    
    // 添加"恢复默认"菜单项
    strcpy(mainMenuItems[mainMenuItemCount].title, "Reset Defaults");
    mainMenuItems[mainMenuItemCount].type = ITEM_NORMAL;
    mainMenuItemCount++;
    
    // 初始化PID参数菜单项
    pidMenuItemCount = 0;
    
    // 获取PID参数
    double kp, ki, kd;
    pidController->getTunings(&kp, &ki, &kd);
    static float pidKp = kp;
    static float pidKi = ki;
    static float pidKd = kd;
    
    // 添加Kp参数项
    strcpy(pidMenuItems[pidMenuItemCount].title, "Kp Value");
    pidMenuItems[pidMenuItemCount].type = ITEM_SLIDER;
    pidMenuItems[pidMenuItemCount].valuePtr = &pidKp;
    pidMenuItems[pidMenuItemCount].minValue = 0.1f;
    pidMenuItems[pidMenuItemCount].maxValue = 100.0f;
    pidMenuItems[pidMenuItemCount].stepValue = 0.5f;
    pidMenuItemCount++;
    
    // 添加Ki参数项
    strcpy(pidMenuItems[pidMenuItemCount].title, "Ki Value");
    pidMenuItems[pidMenuItemCount].type = ITEM_SLIDER;
    pidMenuItems[pidMenuItemCount].valuePtr = &pidKi;
    pidMenuItems[pidMenuItemCount].minValue = 0.0f;
    pidMenuItems[pidMenuItemCount].maxValue = 10.0f;
    pidMenuItems[pidMenuItemCount].stepValue = 0.05f;
    pidMenuItemCount++;
    
    // 添加Kd参数项
    strcpy(pidMenuItems[pidMenuItemCount].title, "Kd Value");
    pidMenuItems[pidMenuItemCount].type = ITEM_SLIDER;
    pidMenuItems[pidMenuItemCount].valuePtr = &pidKd;
    pidMenuItems[pidMenuItemCount].minValue = 0.0f;
    pidMenuItems[pidMenuItemCount].maxValue = 50.0f;
    pidMenuItems[pidMenuItemCount].stepValue = 0.5f;
    pidMenuItemCount++;
    
    // 添加"保存参数"项
    strcpy(pidMenuItems[pidMenuItemCount].title, "Save & Apply");
    pidMenuItems[pidMenuItemCount].type = ITEM_NORMAL;
    pidMenuItemCount++;
    
    // 添加"自动调整"项
    strcpy(pidMenuItems[pidMenuItemCount].title, "Auto Tune");
    pidMenuItems[pidMenuItemCount].type = ITEM_NORMAL;
    pidMenuItemCount++;
    
    // 添加"返回"项
    strcpy(pidMenuItems[pidMenuItemCount].title, "Back");
    pidMenuItems[pidMenuItemCount].type = ITEM_SUBMENU;
    pidMenuItems[pidMenuItemCount].targetPage = UI_PAGE_MENU;
    pidMenuItemCount++;
    
    // 初始化校准菜单项
    calibrationMenuItemCount = 0;
    
    // 添加温度偏移项
    static float tempOffset = tempSensor->getCalibration();
    strcpy(calibrationMenuItems[calibrationMenuItemCount].title, "Temp Offset");
    calibrationMenuItems[calibrationMenuItemCount].type = ITEM_SLIDER;
    calibrationMenuItems[calibrationMenuItemCount].valuePtr = &tempOffset;
    calibrationMenuItems[calibrationMenuItemCount].minValue = -10.0f;
    calibrationMenuItems[calibrationMenuItemCount].maxValue = 10.0f;
    calibrationMenuItems[calibrationMenuItemCount].stepValue = 0.1f;
    calibrationMenuItemCount++;
}

void UIAdapter::setPage(UIPage page) {
    if (page != currentPage) {
        previousPage = currentPage;
        currentPage = page;
        menuSelection = 0; // 重置菜单选择
        valueEditing = false; // 退出编辑模式
    }
}

UIPage UIAdapter::getPage() {
    return currentPage;
}

void UIAdapter::setTemperature(float current, float target) {
    currentTemp = current;
    targetTemp = target;
}

void UIAdapter::setPowerPercentage(uint8_t percentage) {
    powerPercentage = percentage;
}

void UIAdapter::setSystemState(SystemState state) {
    systemState = state;
}

void UIAdapter::showError(ErrorCode code, const char* message) {
    errorCode = code;
    strncpy(errorMessage, message, sizeof(errorMessage) - 1);
    errorMessage[sizeof(errorMessage) - 1] = '\0'; // 确保字符串结束
    
    // 自动切换到错误页面
    setPage(UI_PAGE_ERROR);
}

void UIAdapter::clearError() {
    errorCode = ERROR_NONE;
    strcpy(errorMessage, "");
}

float UIAdapter::getTargetTemp() {
    return targetTemp;
}

void UIAdapter::drawPopup(const char* message, uint16_t duration) {
    // 绘制弹出窗口背景
    display->setDrawColor(1);
    display->drawBox(10, 20, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 40);
    display->setDrawColor(0);
    display->drawBox(12, 22, SCREEN_WIDTH - 24, SCREEN_HEIGHT - 44);
    display->setDrawColor(1);
    
    // 绘制消息
    display->setFont(u8g2_font_ncenB08_tr);
    display->drawStr(15, 35, message);
    
    // 绘制关闭提示
    display->drawStr(15, 50, "Click to close");
}

void UIAdapter::animatePosition(float* pos, float target, float speed) {
    if (*pos != target) {
        if (fabs(*pos - target) <= 1.0f) {
            *pos = target;
        } else {
            *pos += (target - *pos) / (100.0f - speed) / 1.0f;
        }
    }
}