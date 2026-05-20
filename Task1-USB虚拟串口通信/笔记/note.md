## USB虚拟串口通信学习笔记

### 开发板信息

- **型号**: STM32F407VET6
- **内核**: Cortex-M4，最高168 MHz
- **USB**: USB OTG FS（需严格48 MHz）

### CubeMX配置步骤

1. **Debug配置**
   - System Core → Debug → Serial Wire（否则可能无法再次烧录）
2. **RCC设置**
   - System Core → RCC → High Speed Clock (HSE) → Crystal/Ceramic Resonator（8 MHz晶振）
3. **USB配置**
   - Connectivity → USB\_OTG\_FS → Mode = Device Only
   - Middleware → USB\_DEVICE → Class For FS IP = Communication Device Class (CDC)
   - CubeMX自动配置PA11(DM)、PA12(DP)为USB复用
4. **定时器TIM2配置**
   - Timers → TIM2 → 时钟源：Internal Clock
   - Prescaler = 167，Period = 999 → 1ms中断
   - 用于获取运行时长和定时上报数据
5. **GPIO配置**
   - 板载LED：PD2，推挽输出，初始低电平
6. **时钟树配置**
   - PLL\_M = 8
   - PLL\_N = 336
   - PLL\_P = 2
   - PLL\_Q = 7（确保USB OTG FS为48MHz）
7. **工程生成**
   - Toolchain/IDE：CMake
   - 勾选"Generate peripheral initialization as a pair of '.c/.h' files per peripheral"

### VOFA+协议说明

#### JustFloat协议

- 格式：\[通道0(float)]\[通道1(float)]...\[通道N(float)] + 帧尾(0x04 0x05 0x06 0x07)
- 每个通道占用4字节，小端序
- 高效二进制协议，适合实时数据传输

### 代码实现要点

#### 1. 定时器中断（TIM2）

- 1ms中断一次，用于计时和定时上报数据
- 更新系统tick和运行时长

#### 2. USB CDC数据发送

- 使用`CDC_Transmit_FS()`发送数据
- 将float数据通过memcpy转换为字节数组
- 注意发送间隔，避免FIFO忙

#### 3. USB CDC数据接收

- 在`CDC_Receive_FS()`回调中处理接收到的数据
- 将数据复制到全局缓冲区，主循环中解析命令

#### 4. 命令解析

- `led_on`：点亮LED
- `led_off`：熄灭LED
- `led_twinkle`：LED以100ms间隔闪烁
- `sinx`：切换正弦波生成模式

### 关键代码实现

#### main.c

```c
// 定时器中断回调
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        system_tick++;
        run_seconds = (float)system_tick / 10.0f;
        
        if (sinx_mode) {
            sinx_angle += 0.1f;
            float sin_value = sin(sinx_angle);
            Send_JustFloat_Data(run_seconds, sin_value);
        } else {
            Send_JustFloat_Data((float)system_tick, run_seconds);
        }
    }
}

// JustFloat数据发送
void Send_JustFloat_Data(float ch0, float ch1) {
    uint8_t buffer[8];
    memcpy(buffer, &ch0, 4);
    memcpy(buffer + 4, &ch1, 4);
    CDC_Transmit_FS(buffer, 8);
}

// 命令处理
void Process_Command(uint8_t *cmd, uint32_t len) {
    if (strncmp((char*)cmd, "led_on", 6) == 0) {
        LED_On();
    } else if (strncmp((char*)cmd, "led_off", 7) == 0) {
        LED_Off();
    } else if (strncmp((char*)cmd, "led_twinkle", 12) == 0) {
        LED_Twinkle();
    } else if (strncmp((char*)cmd, "sinx", 4) == 0) {
        sinx_mode = !sinx_mode;
    }
}
```

#### usbd\_cdc\_if.c

```c
// 接收回调
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len) {
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    Process_Command(Buf, *Len);
    return (USBD_OK);
}
```

### 调试要点

1. **USB驱动安装**
   - Windows 10/11自动识别虚拟串口
   - 设备管理器中查看COM端口号
   - 烧录：probe-rs run --chip STM32F407VETx .\build\Debug\STM32f407vet6.elf
2. **VOFA+配置**
   - 协议选择：JustFloat
   - 通道绑定：根据数据顺序绑定显示
3. **常见问题**
   - 发送失败：检查USB连接和驱动
   - 数据不显示：确认协议选择正确
   - LED不响应：检查GPIO配置和命令格式

### 学习总结

1. USB CDC本质是通过USB模拟传统串口，上位机无需额外驱动
2. JustFloat协议高效，适合实时数据传输，通过帧尾识别数据包
3. 定时器中断用于精确计时和定时任务
4. 命令解析使用字符串比较函数，注意命令格式匹配
5. HAL库提供了便捷的USB CDC接口函数，简化开发流程

