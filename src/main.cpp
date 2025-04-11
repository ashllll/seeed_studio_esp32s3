#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "temp_sensor.h"
#include "pid_controller.h"
#include "pwm_controller.h"
#include "user_input.h"
#include "ui_adapter.h"

// 模块实例
TempSensor tempSensor;
PIDController pidController;
PWMController pwmController;
UserInput userInput;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
UIAdapter uiAdapter(&display, &tempSensor, &pidController, &pwmController, &userInput);

// 系统状态
SystemState systemState = STATE_IDLE;
ErrorCode errorCode = ERROR_NONE;

// 系统运行时间标记
unsigned long lastSystemStatusUpdateTime = 0;

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial.println("\n\n" SYSTEM_NAME " v" SYSTEM_VERSION);
  Serial.println("系统启动中...");
  
  // 初始化I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000); // 设置为400kHz
  
  // 初始化模块
  Serial.println("初始化硬件模块...");
  
  // 初始化OLED显示屏
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306初始化失败!");
  } else {
    Serial.println("SSD1306初始化成功");
  }
  
  // 显示启动画面
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(SYSTEM_NAME);
  display.setCursor(0, 16);
  display.println("Version: " SYSTEM_VERSION);
  display.setCursor(0, 32);
  display.println("Initializing...");
  display.display();
  
  // 初始化温度传感器
  if (!tempSensor.begin()) {
    Serial.println("温度传感器初始化失败!");
    errorCode = ERROR_TEMP_SENSOR;
  }
  
  // 初始化PID控制器
  if (!pidController.begin()) {
    Serial.println("PID控制器初始化失败!");
  }
  
  // 初始化PWM控制器
  if (!pwmController.begin()) {
    Serial.println("PWM控制器初始化失败!");
  }
  
  // 初始化用户输入
  if (!userInput.begin()) {
    Serial.println("用户输入初始化失败!");
  }
  
  // 初始化UI适配器
  if (!uiAdapter.begin()) {
    Serial.println("UI适配器初始化失败!");
  }
  
  // 如果有错误，更新UI显示
  if (errorCode != ERROR_NONE) {
    systemState = STATE_ERROR;
    if (errorCode == ERROR_TEMP_SENSOR) {
      uiAdapter.showError(errorCode, "Temp sensor error");
    }
  }
  
  // 初始状态设置
  systemState = STATE_IDLE;
  uiAdapter.setSystemState(systemState);
  
  Serial.println("系统初始化完成!");
  Serial.println("单击启动, 双击停止, 长按进入菜单");
  
  // 等待2秒，让用户看到启动画面
  delay(2000);
}

void loop() {
  // 获取当前时间
  unsigned long currentTime = millis();
  
  // 读取温度
  float currentTemp = tempSensor.readTemperature();
  float targetTemp = uiAdapter.getTargetTemp(); // 从UI获取目标温度
  
  // 设置PID控制器的目标温度
  pidController.setTargetTemp(targetTemp);
  
  // 更新UI显示的温度信息
  uiAdapter.setTemperature(currentTemp, targetTemp);
  
  // 每200ms更新一次系统状态
  if (currentTime - lastSystemStatusUpdateTime >= 200) {
    lastSystemStatusUpdateTime = currentTime;
    
    // 安全检查
    if (currentTemp > TEMP_PROTECTION_MAX) {
      // 过温保护
      systemState = STATE_ERROR;
      errorCode = ERROR_OVERTEMP;
      pwmController.emergencyStop();
      uiAdapter.showError(errorCode, "Over temperature");
    }
    
    // 根据系统状态进行处理
    switch (systemState) {
      case STATE_IDLE:
        // 待机状态
        pwmController.disable();
        uiAdapter.setPowerPercentage(0);
        break;
        
      case STATE_WORKING:
        // 工作状态
        // 设置PID输入
        pidController.setCurrentTemp(currentTemp);
        
        // 计算PID输出
        if (pidController.compute()) {
          // 更新PWM输出
          pwmController.setDutyCycle(pidController.getOutput());
          
          // 更新UI显示的功率百分比
          uiAdapter.setPowerPercentage(pwmController.getPowerPercentage());
        }
        break;
        
      case STATE_ERROR:
        // 错误状态
        pwmController.emergencyStop();
        uiAdapter.setPowerPercentage(0);
        break;
        
      default:
        break;
    }
    
    // 更新UI显示的系统状态
    uiAdapter.setSystemState(systemState);
  }
  
  // 处理用户输入
  userInput.update();
  EncoderEvent event = userInput.getEvent();
  
  // 全局事件处理
  switch (event) {
    case EV_SINGLE_CLICK:
      // 单击启动加热（在主界面）
      if (systemState == STATE_IDLE && uiAdapter.getPage() == UI_PAGE_MAIN) {
        systemState = STATE_WORKING;
        pwmController.enable();
        Serial.println("开始加热");
      }
      break;
      
    case EV_DOUBLE_CLICK:
      // 双击停止加热（在任何状态）
      if (systemState == STATE_WORKING) {
        systemState = STATE_IDLE;
        pwmController.disable();
        Serial.println("停止加热");
      }
      break;
      
    case EV_LONG_PRESS:
      // 长按在错误状态下重置
      if (systemState == STATE_ERROR) {
        systemState = STATE_IDLE;
        errorCode = ERROR_NONE;
        Serial.println("错误重置");
      }
      break;
      
    default:
      break;
  }
  
  // 更新UI适配器并处理UI相关输入
  uiAdapter.handleInput();
  uiAdapter.update();
  
  // 短暂延时，避免CPU占用过高
  delay(1);
}