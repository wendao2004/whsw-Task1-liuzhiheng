##  开发板
    F407VET6
    内核：Cortex‑M4，最高 168 MHz
    USB：USB OTG FS（需严格 48 MHz）

# CubeMax步骤
    1. 打开CubeMax，选择芯片型号
    2. 配置时钟树
    3. 配置USB
    4. 配置GPIO
    5. 生成代码
    6. 打开编译器，编译代码
    7. 下载代码
    8. 打开VOFA，连接虚拟串口
    9. 打开串口助手，连接虚拟串口，发送数据
   
 1. stm32f407vet6:
    1. 踩坑：先把 Debug 打开：
        System Core → Debug → 选 Serial Wire（否则下一次可能烧不进去）。
    2. RCC 设置
        System Core → RCC → High Speed Clock (HSE) → 选 Crystal/Ceramic Resonator（8 MHz 晶振）。
    3. Connectivity → USB_OTG_FS → Mode = Device Only
        Middleware → USB_DEVICE → Class For FS IP = Communication Device Class (CDC)
        保持默认（端点、缓冲区大小默认即可）。
        此时 CubeMX 会自动把 PA11=DM、PA12=DP 设为 USB 复用，无需手动改 GPIO。
    4. 定时器 TIM2 配置（1 ms 中断，计时用）
        目的：获取上电毫秒数、运行时长、LED 闪烁。
        Timers → TIM2 → 时钟源：Internal Clock


# CubeMX注意项
### USB外设配置:
* Connectivity -> USB_OTG_FS -> Device_Only
* Middleware -> USB_Device -> CDC

### USB_OTG_FS时钟为48Mhz
#### F407 HSE PLL:
* pll_m = 8; 
* pll_n = 336; 
* pll_p = 2; 
* pll_q = 7;

### GPIO中断配置:
 * 自动配置pa11(usb_dm),pa12(usb_dp)
* 开启「USB_OTG_FS global interrupt」，优先级设为2（高于SysTick，避免丢包）
*  配置板载LED对应的GPIO(F407VET6
核心板是Pd2/Pd3，推挽输出，初始低电平）

### 工程生成配置:
  * 「Project Manager」→「Toolchain/IDE」选「CMake」
  * 「Code Generator」→ 勾选
「Generate peripheral initialization as a pair of '.c/.h' files per peripheral」
  * 重新生成代码，覆盖原有工程
  
 ### CDC 收发基础封装:
   * USB CDC的核心收发函数
是 CDC_Transmit_FS()，接收回调 在 CDC_Receive_FS()，需做全局缓冲区转发（禁止在中断回调中直接处理业务。
   
### 全局缓冲(main.c)
```
#include "usbd_cdc_if.h"
#include <string.h>
#include <math.h>
// 接收缓冲区（最大64字节，对应USB端点大小）
uint8_t uart_rx_buf[64] = {0};
uint8_t uart_rx_len = 0;
理
uint8_t uart_rx_flag = 0; // 1＝有新数据待处
// 定时上报变量
uint32_t last_tick = 0;
10
float run_seconds = 0.0f;
11
float sin_value = 0.0f;
12
uint8_t sin_enable = 0; // 正弦波生成开关
关 uint8_t led_twinkle_flag = 0; // LED闪烁开
14 uint32_t led_last_tick = 0;

```
### 修改接收回调(usbd_cdc_if.c)
```
/／找到这个函数，替换原有内容 
extern uint8_t uart_rx_buf[64];
extern uint8_t uart_rx_len;
extern uint8_t uart_rx_flag;
static int8_t CDC_Receive_FS(uint8_t* Buf,uint32_t *Len)
{
// 复制数据到全局缓冲区，限制长度
memcpy(uart_rx_buf, Buf, uart_rx_len);uart_rx_len =(*Len > 64)? 64 : *Len;
memcpy(uart_rx_buf,Buf,uart_rx_len);
uart_rx_flag = 1;// 置位接收标志
//重新开启接收（必须加，否则只能收一次）USBD_CDC_SetRxBuffer(&hUsbDeviceFS,
Buf);
USBD_CDC_ReceivePacket(&hUsbDeviceFS);
return(USBD_OK);
}
```
### 封装安全发送函数(main.c)
```
 void usb_cdc_send(uint8_t *data, uint16_t len)
{
  uint32_t timeout = HAL_GetTick() + 100; // 100ms超时
  while(CDC_Transmit_FS(data, len) != USBD_OK)
  {
    if(HAL_GetTick() > timeout)  
      break;
  }
}
```
### JustFloat 格式
```
[通道0(float)][通道1(float)]...[通道N(float)] + 帧尾(0x04 0x05 0x06 0x07)
```
### main.c
```
// JustFloat协议打包发送
void vofa_send_justfloat(float *ch_data, uint8_t ch_num)
{
  uint8_t buf[128] = {0};
  uint16_t len = ch_num * 4;
  
  // 复制浮点数数据（STM32小端，直接 memcpy 即可）
  memcpy(buf, ch_data, len);
  
  // 添加固定帧尾
  buf[len++] = 0x04;
  buf[len++] = 0x05;
  buf[len++] = 0x06;
  buf[len++] = 0x07;
  
  // 发送数据
  usb_cdc_send(buf, len);
}
```
### 主循环
```
#include "main.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// ---------------------- 全局变量 ----------------------
uint8_t  uart_rx_buf[64];  // 接收缓冲区
uint32_t uart_rx_len;      // 接收长度
uint8_t  rx_done = 0;      // 接收完成标志

float run_sec = 0.0f;
uint32_t tick_send = 0;

 int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  
  last_tick = HAL_GetTick();
  
  while (1)
  {
    // 1. 每100ms上报一次数据（任务要求）
    if(HAL_GetTick() - last_tick >= 100)
    {
      last_tick = HAL_GetTick();
      run_seconds = (float)HAL_GetTick() / 1000.0f; // 运行时长，精确到ms
      
      // 必做：通道0=时间戳(用系统tick代替RTC，简单可靠)，通道1=运行时长
      float ch_data[3] = {0};
      ch_data[0] = (float)HAL_GetTick(); // 通道1（VOFA+显示）
      ch_data[1] = run_seconds;          // 通道2（VOFA+显示）
      
      // 加分项：生成正弦波
      if(sin_enable)
      {
        sin_value = sinf(run_seconds * 2.0f * 3.14159f); // 1Hz正弦波
        ch_data[2] = sin_value; // 通道3
        vofa_send_justfloat(ch_data, 3);
      }
      else
      {
        vofa_send_justfloat(ch_data, 2);
      }
      
      // 加分项：LED闪烁
      if(led_twinkle_flag && (HAL_GetTick() - led_last_tick >= 100))
      {
        led_last_tick = HAL_GetTick();
        HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9); // 替换为你的LED引脚
      }
    }
    
    // 2. 处理上位机命令
    if(uart_rx_flag)
    {
      uart_rx_flag = 0;
      // 去除末尾的换行符/回车（VOFA+发送时可能自动添加）
      uart_rx_buf[uart_rx_len] = '\0';
      if(uart_rx_buf[uart_rx_len-1] == '\n' || uart_rx_buf[uart_rx_len-1] == '\r')
      {
        uart_rx_buf[uart_rx_len-1] = '\0';
      }
      
      // 命令解析
      if(strcmp((char*)uart_rx_buf, "led_on") == 0)
      {
        led_twinkle_flag = 0;
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET); // 亮
      }
      else if(strcmp((char*)uart_rx_buf, "led_off") == 0)
      {
        led_twinkle_flag = 0;
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_SET); // 灭
      }
      else if(strcmp((char*)uart_rx_buf, "led_twinkle") == 0)
      {
        led_twinkle_flag = 1;
        led_last_tick = HAL_GetTick();
      }
      else if(strcmp((char*)uart_rx_buf, "sinx") == 0)
      {
        sin_enable = !sin_enable; // 切换正弦波开关
      }
      
      // 清空接收缓冲区
      memset(uart_rx_buf, 0, 64);
      uart_rx_len = 0;
    }
  }
}

```

# CDC封装 
### CDC-AM
* ACM (发送控制命令）
* 数据接口（收发用户数据)In 设备→主机
* 数据接口（收发用户数据out 主机→设备

### 避免超时
```
// 超时重发函数，就是解决"发送FIFO忙"的问题
void usb_cdc_send(uint8_t *data, uint16_t len)
{
  uint32_t timeout = HAL_GetTick() + 100;
  // 如果上一包还没发完，就等一会儿，超时就放弃
  while(CDC_Transmit_FS(data, len) != USBD_OK)
  {
    if(HAL_GetTick() > timeout) break;
  }
}
```
### 总结：CDC 核心要点
1.  本质：STM32通过USB模拟传统串口，上位机无需额外驱动
​
2.  结构：2个接口+3个端点，批量IN发数据，批量OUT收数据
​
3. 发送：调用 CDC_Transmit_FS() ，间隔≥10ms，加超时重试
​
4. 接收：回调里复制数据+置标志+重开接收，主循环里加'\0'再解析
​
5. 协议：VOFA+通过帧尾识别数据包，JustFloat高效，FireWater易调试