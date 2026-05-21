# Task1 - USB虚拟串口通信笔记

## 开发板

STM32F407VET6，用probe-rs烧录

## 硬件连接

- 烧录用ST-Link接口
- 通信用Type-C接口（USB OTG）
- LED在PA5引脚上

## CubeMX配置

### 基础配置

- Debug：Serial Wire（不然下次烧录会出问题）
- RCC：HSE选Crystal/Ceramic Resonator
- 时钟：8MHz晶振，PLL配置168MHz主频

### USB配置

- USB_OTG_FS → Mode = Device Only
- USB_DEVICE → Class选CDC
- PA11/PA12自动配置成USB引脚

### 定时器

- TIM2，时钟源Internal Clock
- Prescaler=167，Period=999（1ms中断）

### 注意

- USB需要严格的48MHz时钟，PLL_Q要设为7
- LED从PD2改成了PA5

## 代码实现

### 全局变量

```c
uint32_t system_tick = 0;      // 系统运行滴答
float run_seconds = 0.0f;     // 运行时长（秒）
uint8_t led_twinkle_mode = 0; // LED闪烁模式
uint8_t sinx_mode = 0;        // 正弦波模式
float sinx_angle = 0.0f;      // 正弦波角度
uint8_t protocol_mode = 0;   // 0=JustFloat, 1=FireWater
```

### 主循环逻辑

```c
while (1) {
    static uint32_t last_send = 0;
    if (HAL_GetTick() - last_send >= 100) {
        last_send = HAL_GetTick();
        run_seconds = (float)last_send / 1000.0f;
        
        if (sinx_mode) {
            sinx_angle += 0.1f;
            float sin_value = sin(sinx_angle);
            Send_JustFloat_Data(run_seconds, sin_value);
        } else {
            Send_JustFloat_Data(run_seconds, run_seconds);
        }
    }
    
    if (led_twinkle_mode) {
        // LED闪烁控制
    }
}
```

### 发送函数（支持两种协议）

```c
void Send_JustFloat_Data(float ch0, float ch1) {
    if (protocol_mode == 0) {
        // JustFloat二进制协议
        uint8_t buffer[12];
        memcpy(buffer, &ch0, 4);
        memcpy(buffer + 4, &ch1, 4);
        buffer[8] = 0x00;
        buffer[9] = 0x00;
        buffer[10] = 0x80;
        buffer[11] = 0x7f;
        CDC_Transmit_FS(buffer, 12);
    } else {
        // FireWater文本协议
        char buffer[64];
        uint32_t hours = (uint32_t)(run_seconds / 3600);
        uint32_t minutes = (uint32_t)(run_seconds / 60) % 60;
        uint32_t seconds = (uint32_t)run_seconds % 60;
        
        uint16_t len = sprintf(buffer, "2026-05-19 %02lu:%02lu:%02lu %.2f\n",
                              hours, minutes, seconds, run_seconds);
        CDC_Transmit_FS((uint8_t*)buffer, len);
    }
}
```

### 命令处理

```c
void Process_Command(uint8_t *cmd, uint32_t len) {
    if (len > 0 && len < APP_RX_DATA_SIZE) {
        cmd[len] = '\0';  // 必须加结束符，不然字符串函数会出问题
    }
    
    if (strncmp((char*)cmd, "led_on", 6) == 0) {
        LED_On();
    } else if (strncmp((char*)cmd, "led_off", 7) == 0) {
        LED_Off();
    } else if (strncmp((char*)cmd, "led_twinkle", 11) == 0) {
        LED_Twinkle();
    } else if (strncmp((char*)cmd, "sinx", 4) == 0) {
        sinx_mode = !sinx_mode;
        if (sinx_mode) sinx_angle = 0.0f;
    } else if (strncmp((char*)cmd, "proto_justfloat", 16) == 0) {
        protocol_mode = 0;
    } else if (strncmp((char*)cmd, "proto_firewater", 16) == 0) {
        protocol_mode = 1;
    }
}
```

### USB接收回调

```c
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len) {
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    Process_Command(Buf, *Len);
    return (USBD_OK);
}
```

## 函数说明

### HAL库函数

#### HAL_GetTick()
- 获取系统启动后的毫秒数
- 返回值：uint32_t类型，单位毫秒
- 用法：用于定时判断
```c
if (HAL_GetTick() - last_send >= 100) {
    // 过了100ms
}
```

#### HAL_GPIO_WritePin()
- 设置GPIO引脚电平
- 参数：GPIOx（端口）、GPIO_Pin（引脚）、PinState（状态）
- 用法：点亮或熄灭LED
```c
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);  // 点亮
HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);  // 熄灭
```

#### HAL_GPIO_TogglePin()
- 翻转GPIO引脚电平
- 用法：LED闪烁
```c
HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);
```

#### HAL_TIM_Base_Start_IT()
- 启动定时器中断模式
- 参数：定时器句柄
- 用法：启动TIM2中断
```c
HAL_TIM_Base_Start_IT(&htim2);
```

### USB CDC函数

#### CDC_Transmit_FS()
- 通过USB虚拟串口发送数据
- 参数：数据缓冲区、数据长度
- 用法：发送数据到VOFA+
```c
CDC_Transmit_FS(buffer, 12);
```

### 标准库函数

#### strncmp()
- 比较两个字符串的前n个字符
- 返回值：0表示相等
- 用法：命令解析
```c
if (strncmp((char*)cmd, "led_on", 6) == 0) {
    // 执行命令
}
```

#### sprintf()
- 格式化字符串输出到缓冲区
- 用法：生成文本日志
```c
sprintf(buffer, "%02lu:%02lu:%02lu %.2f\n", hours, minutes, seconds, run_seconds);
```

#### memcpy()
- 内存复制
- 用法：将float复制到字节数组
```c
memcpy(buffer, &ch0, 4);
```

#### sin()
- 计算正弦值（弧度）
- 用法：生成正弦波数据
```c
float sin_value = sin(angle);
```

### 自定义函数

#### LED_On() / LED_Off() / LED_Twinkle()
- LED控制函数，分别实现点亮、熄灭、闪烁切换

#### Send_JustFloat_Data()
- 发送数据到VOFA+，支持JustFloat和FireWater两种协议

#### Process_Command()
- 处理VOFA+发来的命令，解析并执行相应操作

## VOFA+使用

### JustFloat模式（看波形）

1. 协议选JustFloat
2. 新建仪表板，添加示波器
3. 添加两个通道：数据0、数据1
4. 在仪表板看波形，不要在终端看（终端显示乱码是正常的）

### FireWater模式（看日志）

1. 发送命令`proto_firewater`
2. 协议改成FireWater
3. 在终端看文本日志

输出格式：
```
2026-05-19 00:00:00 0.00
2026-05-19 00:00:01 1.00
2026-05-19 00:02:03 123.45
```

## 遇到的问题

1. **Type-C插上不显示COM口**
   - 不要插烧录口

2. **VOFA+看不到数据**
   - JustFloat协议终端显示乱码是正常的，要在仪表板看
   - FireWater要在终端看

3. **烧录后灯不闪**
   - 检查GPIO配置是否正确，PA5要设为输出模式
   - 检查代码是否正确烧录，是否有错误

4. **命令不响应**
   - 检查命令格式是否正确
   - 记得加`\0`结束符

## 总结

- USB CDC就是模拟串口，Windows自动装驱动
- JustFloat是二进制协议，适合看波形
- FireWater是文本协议，适合看日志
- 两种协议可以动态切换，一个固件搞定
- 发送间隔不要太短，100ms比较稳
