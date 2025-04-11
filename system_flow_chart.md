# ESP32-S3 温控系统流程图（增强版）

```mermaid
graph TD
    %% 系统初始化
    Start[系统启动] --> InitSerial[初始化串口]
    InitSerial --> InitI2C[初始化I2C]
    InitI2C --> InitDisplay[初始化OLED]
    InitDisplay --> ShowSplash[显示启动界面]
    ShowSplash --> InitModules[初始化模块<br/>温度传感器/PID/PWM/输入]
    InitModules --> InitUIAdapter[初始化UI适配器]
    InitUIAdapter --> NetInterface[初始化网络接口预留]
    NetInterface --> ErrorCheck{检查错误?}
    ErrorCheck -->|有错误| ShowError[显示错误页面<br/>设置错误状态]
    ErrorCheck -->|无错误| SetIdle[设置待机状态]
    ShowError --> SetIdle
    SetIdle --> MainLoop[进入主循环]

    %% 主循环
    MainLoop --> ReadTemp[读取当前温度]
    ReadTemp --> SensorCheck{传感器检查}
    SensorCheck -->|错误| HandleSensorError[传感器错误处理]
    SensorCheck -->|正常| GetTargetTemp[获取目标温度<br/>从UI适配器]
    
    HandleSensorError --> ErrorDisplay[显示错误信息]
    ErrorDisplay --> SafetyMeasures[采取安全措施]
    SafetyMeasures --> UpdateUIState
    
    GetTargetTemp --> TempRateCheck[温度变化率检查]
    TempRateCheck -->|变化率过高| RateProtection[温度变化率保护]
    TempRateCheck -->|正常| UpdatePID[更新PID目标温度]
    
    RateProtection --> PWMLimiter[PWM功率限制]
    PWMLimiter --> UpdatePID
    
    UpdatePID --> UpdateUITemp[更新UI温度显示]
    UpdateUITemp --> TimeCheck{刷新间隔检查}
    
    %% 自适应刷新率
    TimeCheck -->|主页面/快速更新| ShortInterval[100ms间隔更新]
    TimeCheck -->|菜单页面/慢速更新| LongInterval[200ms间隔更新]
    ShortInterval --> SafetyCheck
    LongInterval --> SafetyCheck
    
    %% 系统状态更新
    SafetyCheck{安全检查} -->|过温/异常| EmergencyStop[紧急停止<br/>显示错误]
    SafetyCheck -->|正常| StateSwitch{系统状态?}
    
    StateSwitch -->|待机| IdleProcess[禁用PWM<br/>功率设为0]
    StateSwitch -->|工作| WorkingProcess[PID计算<br/>更新PWM输出]
    StateSwitch -->|错误| ErrorProcess[紧急停止<br/>功率设为0]
    
    IdleProcess --> UpdateUIState[更新UI系统状态]
    WorkingProcess --> UpdateUIState
    ErrorProcess --> UpdateUIState
    EmergencyStop --> UpdateUIState
    
    UpdateUIState --> ReadInput[处理用户输入]
    
    %% 全局事件处理
    ReadInput --> EventSwitch{事件类型?}
    EventSwitch -->|单击| ClickHandler[主页面单击<br/>启动加热]
    EventSwitch -->|双击| DoubleClickHandler[停止加热]
    EventSwitch -->|长按| LongPressHandler[错误状态下重置]
    EventSwitch -->|旋转| RotateHandler[调整参数]
    
    ClickHandler --> UIProcess
    DoubleClickHandler --> UIProcess
    LongPressHandler --> UIProcess
    RotateHandler --> UIProcess
    
    %% UI处理流程
    UIProcess[UI适配器处理输入] --> UIPageSwitch{当前UI页面?}
    
    UIPageSwitch -->|主页面| MainPageHandler[处理主页面输入<br/>温度调整/菜单进入]
    UIPageSwitch -->|菜单页面| MenuPageHandler[处理菜单导航<br/>选择/编辑/返回]
    UIPageSwitch -->|PID页面| PIDPageHandler[处理PID参数<br/>调整/保存]
    UIPageSwitch -->|校准页面| CalibrationHandler[处理校准<br/>偏移调整]
    UIPageSwitch -->|系统信息| SystemInfoHandler[显示系统信息]
    UIPageSwitch -->|错误页面| ErrorPageHandler[显示错误信息]
    
    MainPageHandler --> UpdateUI[更新UI显示]
    MenuPageHandler --> UpdateUI
    PIDPageHandler --> UpdateUI
    CalibrationHandler --> UpdateUI
    SystemInfoHandler --> UpdateUI
    ErrorPageHandler --> UpdateUI
    
    UpdateUI --> ShortDelay[短暂延时]
    ShortDelay --> MainLoop
    
    %% UI绘制流程子图
    subgraph "UI绘制流程"
        UpdateUI --> ClearDisplay[清除显示]
        ClearDisplay --> AnimationUpdate[更新动画帧]
        AnimationUpdate --> DrawPage{绘制当前页面}
        
        DrawPage -->|主页面| DrawMainPage[绘制主页面<br/>温度/功率/状态]
        DrawPage -->|菜单页面| DrawMenu[绘制菜单项<br/>选择器/面包屑/提示]
        DrawPage -->|校准页面| DrawCalibration[绘制校准页面<br/>当前/目标/偏移]
        DrawPage -->|错误页面| DrawError[绘制错误页面<br/>错误代码/消息]
        
        DrawMainPage --> DisplayUpdate[刷新显示]
        DrawMenu --> DisplayUpdate
        DrawCalibration --> DisplayUpdate
        DrawError --> DisplayUpdate
    end
    
    %% PID自整定流程
    subgraph "PID自整定流程"
        AutoTune[开始自整定] --> SetTestTemp[设置测试温度]
        SetTestTemp --> MonitorResponse[监测系统响应]
        MonitorResponse --> CalculateParams[计算PID参数]
        CalculateParams --> TestParams[测试参数效果]
        TestParams --> AdjustParams[调整参数]
        AdjustParams --> FinalizeParams[确定最终参数]
        FinalizeParams --> SaveParams[保存参数到EEPROM]
    end
    
    %% 增强错误检测
    subgraph "错误检测系统"
        ErrorDetection[错误检测] --> SensorOpenCheck[传感器开路检测]
        ErrorDetection --> SensorShortCheck[传感器短路检测]
        ErrorDetection --> ADCErrorCheck[ADC读取异常检测] 
        ErrorDetection --> TempRateErrorCheck[温度变化率异常检测]
        
        SensorOpenCheck -->|异常| SensorError[传感器错误]
        SensorShortCheck -->|异常| SensorError
        ADCErrorCheck -->|异常| ADCError[ADC错误]
        TempRateErrorCheck -->|异常| RateError[变化率错误]
        
        SensorError --> SystemErrorState[系统错误状态]
        ADCError --> SystemErrorState
        RateError --> SystemErrorState
    end
    
    %% 菜单结构
    subgraph "菜单结构"
        MainMenu[主菜单] --> PIDMenu[PID参数菜单]
        MainMenu --> CalibrationMenu[校准菜单]
        MainMenu --> SystemInfo[系统信息]
        MainMenu --> HeatingControl[加热控制开关]
        MainMenu --> ResetDefault[恢复默认]
        
        PIDMenu --> KpValue[Kp值调整]
        PIDMenu --> KiValue[Ki值调整]
        PIDMenu --> KdValue[Kd值调整]
        PIDMenu --> SavePID[保存应用]
        PIDMenu --> AutoTuneMenu[自动整定]
        PIDMenu --> BackToMain[返回主菜单]
        
        AutoTuneMenu --> StartAutoTune[开始自整定]
        AutoTuneMenu --> CancelAutoTune[取消并返回]
    end
    
    %% 视觉样式
    classDef initPhase fill:#d4f7d4,stroke:#333,stroke-width:1px;
    classDef mainProcess fill:#d4e7f7,stroke:#333,stroke-width:1px;
    classDef safetyProcess fill:#f7d4d4,stroke:#333,stroke-width:1px;
    classDef userInterface fill:#f7f7d4,stroke:#333,stroke-width:1px;
    classDef menuStructure fill:#f5e1ff,stroke:#333,stroke-width:1px;
    classDef errorProcess fill:#ffcccb,stroke:#333,stroke-width:1px;
    classDef autoTuneProcess fill:#d8bfd8,stroke:#333,stroke-width:1px;
    
    class Start,InitSerial,InitI2C,InitDisplay,ShowSplash,InitModules,InitUIAdapter initPhase;
    class MainLoop,ReadTemp,GetTargetTemp,UpdatePID,TimeCheck,StateSwitch mainProcess;
    class SafetyCheck,EmergencyStop,ErrorProcess,TempRateCheck,RateProtection safetyProcess;
    class UIProcess,UIPageSwitch,UpdateUI,MainPageHandler,MenuPageHandler userInterface;
    class MainMenu,PIDMenu,CalibrationMenu,SystemInfo,HeatingControl menuStructure;
    class ErrorDetection,SensorOpenCheck,SensorShortCheck,ADCErrorCheck,SensorError,ADCError errorProcess;
    class AutoTune,SetTestTemp,MonitorResponse,CalculateParams,TestParams,AdjustParams autoTuneProcess;
```

## 最新流程说明

### 主要功能增强

1. **增强错误检测系统**
   - 添加传感器开路检测：监测NTC100K传感器连接状态
   - 添加传感器短路检测：监测NTC100K是否短路
   - 添加ADC读取异常检测：监测ADS1115读数是否正常
   - 完善错误处理流程：根据错误类型采取不同安全措施

2. **温度变化率监控**
   - 实时监测温度变化速率：检查温度上升或下降是否过快
   - 自适应功率限制：当变化率过高时限制PWM功率输出
   - 防止加热失控：减少温度过冲并保护加热元件

3. **UI响应性优化**
   - 自适应刷新率策略：根据不同页面需求调整刷新间隔
   - 主页面/关键数据：使用较快刷新率保证实时性
   - 菜单页面/静态内容：使用较慢刷新率减少资源占用
   - 考虑SSD1306刷新特性优化显示更新

4. **菜单导航优化**
   - 添加导航面包屑：显示当前位置和层级关系
   - 增强视觉提示：突出显示可选项和当前位置
   - 直观的操作提示：在界面底部显示操作指南

5. **完整的PID自整定流程**
   - 集成完善的PID算法库：利用成熟算法进行参数整定
   - 测试-监测-调整流程：自动进行系统响应测试
   - 一键自整定：用户友好的自整定启动和监控界面
   - 参数存储：自动将优化后的参数保存到EEPROM

6. **网络功能接口预留**
   - 初始化阶段预留网络接口
   - 为后续功能扩展提供便利
   - 不影响当前核心温控功能

### UI界面增强

1. **主页面增强**
   - 添加温度变化率指示器
   - 优化动画进度条视觉效果
   - 增加系统状态图标指示器

2. **菜单页面增强**
   - 添加导航面包屑显示当前路径
   - 优化选择器视觉效果
   - 增加参数调整提示和范围显示

3. **自整定页面**
   - 显示自整定进度和阶段
   - 实时显示测试温度和系统响应
   - 完成后显示新旧参数对比 