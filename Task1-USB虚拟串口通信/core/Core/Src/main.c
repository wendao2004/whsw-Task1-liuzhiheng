/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <string.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint32_t system_tick = 0;
float run_seconds = 0.0f;
uint8_t led_state = 0;
uint8_t led_twinkle_mode = 0;
uint8_t sinx_mode = 0;
float sinx_angle = 0.0f;
uint16_t send_counter = 0;
uint16_t led_twinkle_counter = 0;
uint8_t usb_sent = 0;
uint8_t protocol_mode = 0;  // 0=JustFloat, 1=FireWater
extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);// 配置系统时钟
static void MX_GPIO_Init(void);// 初始化GPIO引脚
static void MX_TIM2_Init(void);// 初始化定时器2
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();// 初始化HAL库

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();// 配置系统时钟

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();// 初始化GPIO引脚
  MX_TIM2_Init();// 初始化定时器2
  MX_USB_DEVICE_Init();// 初始化USB设备
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim2);// 启动定时器2中断
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 数据定时发送
    // 每 100ms 触发一次数据发送：
    // 正弦模式：计算正弦值，发送「运行时间 + 正弦值」
    // 普通模式：发送「系统计时 + 运行秒数」
    static uint32_t last_send = 0;
    if (HAL_GetTick() - last_send >= 100) {
      last_send = HAL_GetTick();// 更新上次发送时间戳
      run_seconds = (float)last_send / 1000.0f; // 计算运行时间，单位秒
      // 发送数据
      if (sinx_mode) {
        sinx_angle += 0.1f;// 正弦角度自增0.1度
        float sin_value = sin(sinx_angle);// 计算正弦值
        Send_JustFloat_Data(run_seconds, sin_value);// 发送运行时间 + 正弦值
      } else {
        Send_JustFloat_Data(run_seconds, run_seconds);// 发送系统计时 + 运行秒数
      }
    }
    
    if (led_twinkle_mode) {
      static uint32_t last_toggle = 0;// 上次切换时间戳
      if (HAL_GetTick() - last_toggle >= 100) {
        last_toggle = HAL_GetTick();// 更新上次切换时间戳
        HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);// 切换LED状态
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 167;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)// 初始化GPIO引脚
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};// GPIO初始化结构体，用于配置GPIO引脚的属性
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();// 使能GPIOH时钟
  __HAL_RCC_GPIOA_CLK_ENABLE();// 使能GPIOA时钟
  __HAL_RCC_GPIOD_CLK_ENABLE();// 使能GPIOD时钟

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);// 初始化LED引脚为低电平，关闭LED

  /*Configure GPIO pin : led_Pin */
  GPIO_InitStruct.Pin = led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(led_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// TIM2 定时器中断回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {// 定时器2中断回调函数
  if (htim->Instance == TIM2) {
    system_tick++;// 系统计时器增加1
    run_seconds = (float)system_tick / 1000.0f;   // 计算运行时间，单位秒
    send_counter++;// 发送计数器增加1

    if (send_counter >= 100) {
      send_counter = 0;// 重置发送计数器
      if (sinx_mode) {
        sinx_angle += 0.1f;// 增加正弦角度
        float sin_value = sin(sinx_angle);// 计算正弦值
        Send_JustFloat_Data(run_seconds, sin_value);//发送「运行时间 + 正弦值」
      } else {
        Send_JustFloat_Data((float)system_tick, run_seconds);//发送「系统计时 + 运行秒数」
      }
    }
//LED 闪烁：开启 led_twinkle_mode 时，每 100ms 翻转一次 LED 电平，实现闪烁。
    if (led_twinkle_mode) {
      led_twinkle_counter++;
      if (led_twinkle_counter >= 100) {
        led_twinkle_counter = 0;//重置闪烁计数器
        HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);//切换 LED 电平，实现闪烁
      }
    }
  }
}

// 点亮 LED 灯，关闭闪烁模式
void LED_On(void) {
  led_twinkle_mode = 0;//禁用 LED 闪烁模式（led_twinkle_mode=0）
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_SET);//硬件置位 LED 引脚，点亮 LED
  led_state = 1;//标记 LED 当前状态为开启（led_state=1）
}
//实现 LED 灯关闭操作
void LED_Off(void) {
  led_twinkle_mode = 0;//禁用 LED 闪烁模式（led_twinkle_mode=0）
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);//硬件置位 LED 引脚，熄灭 LED
  led_state = 0;//标记 LED 当前状态为关闭（led_state=0）
}
//切换 LED 闪烁模式
void LED_Twinkle(void) {
    led_twinkle_mode = !led_twinkle_mode;  // 切换模式
    if (led_twinkle_mode) {
        led_twinkle_counter = 0;//重置闪烁计数器
    }
}

//发送 JustFloat 数据
void Send_JustFloat_Data(float ch0, float ch1) {
    if (protocol_mode == 0) {
        uint8_t buffer[12];// 12字节缓冲区，用于存储 JustFloat 数据
        memcpy(buffer, &ch0, 4);// 复制 ch0 到缓冲区前 4 个字节
        memcpy(buffer + 4, &ch1, 4);// 复制 ch1 到缓冲区后 4 个字节
        buffer[8] = 0x00;// 填充 0x00 字节
        buffer[9] = 0x00;// 填充 0x00 字节
        buffer[10] = 0x80;// 填充 0x80 字节
        buffer[11] = 0x7f;// 填充 0x7f 字节
        CDC_Transmit_FS(buffer, 12);//发送 JustFloat 数据
    }
//发送 FireWater 数据
     else {
        char buffer[64];
        uint32_t hours = (uint32_t)(run_seconds / 3600);//计算小时数
        uint32_t minutes = (uint32_t)(run_seconds / 60) % 60;//计算分钟数
        uint32_t seconds = (uint32_t)run_seconds % 60;//计算秒数
        
        uint16_t len = sprintf(buffer, "2026-05-19 %02lu:%02lu:%02lu %.2f\n",
                              hours, minutes, seconds, run_seconds);//格式化字符串
        CDC_Transmit_FS((uint8_t*)buffer, len);//发送 FireWater 数据
    }
}
//处理命令
void Process_Command(uint8_t *cmd, uint32_t len) {
  if (len > 0 && len < APP_RX_DATA_SIZE) {
    cmd[len] = '\0';//添加字符串结束符
  }
  
  if (strncmp((char*)cmd, "led_on", 6) == 0) {
    LED_On();//点亮 LED 灯
  } else if (strncmp((char*)cmd, "led_off", 7) == 0) {
    LED_Off();//熄灭 LED 灯
  } else if (strncmp((char*)cmd, "led_twinkle", 12) == 0) {
    LED_Twinkle();//切换 LED 闪烁模式
  } else if (strncmp((char*)cmd, "sinx", 4) == 0) {
    sinx_mode = !sinx_mode;//切换正弦模式
    if (sinx_mode) {
      sinx_angle = 0.0f;//重置正弦角度
    }
  } else if (strncmp((char*)cmd, "proto_justfloat", 16) == 0) {
    protocol_mode = 0;//切换协议 JustFloat
  } else if (strncmp((char*)cmd, "proto_firewater", 16) == 0) {
    protocol_mode = 1;//切换协议 FireWater
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
