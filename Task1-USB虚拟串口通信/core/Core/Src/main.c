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
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
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
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
      static uint32_t last_toggle = 0;
      if (HAL_GetTick() - last_toggle >= 100) {
        last_toggle = HAL_GetTick();
        HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);
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
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);

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
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
    system_tick++;
    run_seconds = (float)system_tick / 1000.0f;
    
    send_counter++;
    if (send_counter >= 100) {
      send_counter = 0;
      if (sinx_mode) {
        sinx_angle += 0.1f;
        float sin_value = sin(sinx_angle);
        Send_JustFloat_Data(run_seconds, sin_value);
      } else {
        Send_JustFloat_Data((float)system_tick, run_seconds);
      }
    }
    
    if (led_twinkle_mode) {
      led_twinkle_counter++;
      if (led_twinkle_counter >= 100) {
        led_twinkle_counter = 0;
        HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);
      }
    }
  }
}

void LED_On(void) {
  led_twinkle_mode = 0;
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_SET);
  led_state = 1;
}

void LED_Off(void) {
  led_twinkle_mode = 0;
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);
  led_state = 0;
}

void LED_Twinkle(void) {
    led_twinkle_mode = !led_twinkle_mode;  // 切换模式
    if (led_twinkle_mode) {
        led_twinkle_counter = 0;
    }
}

void Send_JustFloat_Data(float ch0, float ch1) {
    if (protocol_mode == 0) {
        uint8_t buffer[12];
        memcpy(buffer, &ch0, 4);
        memcpy(buffer + 4, &ch1, 4);
        buffer[8] = 0x00;
        buffer[9] = 0x00;
        buffer[10] = 0x80;
        buffer[11] = 0x7f;
        CDC_Transmit_FS(buffer, 12);
    } else {
        char buffer[64];
        uint32_t hours = (uint32_t)(run_seconds / 3600);
        uint32_t minutes = (uint32_t)(run_seconds / 60) % 60;
        uint32_t seconds = (uint32_t)run_seconds % 60;
        
        uint16_t len = sprintf(buffer, "2026-05-19 %02lu:%02lu:%02lu %.2f\n",
                              hours, minutes, seconds, run_seconds);
        CDC_Transmit_FS((uint8_t*)buffer, len);
    }
}

void Process_Command(uint8_t *cmd, uint32_t len) {
  if (len > 0 && len < APP_RX_DATA_SIZE) {
    cmd[len] = '\0';
  }
  
  if (strncmp((char*)cmd, "led_on", 6) == 0) {
    LED_On();
  } else if (strncmp((char*)cmd, "led_off", 7) == 0) {
    LED_Off();
  } else if (strncmp((char*)cmd, "led_twinkle", 12) == 0) {
    LED_Twinkle();
  } else if (strncmp((char*)cmd, "sinx", 4) == 0) {
    sinx_mode = !sinx_mode;
    if (sinx_mode) {
      sinx_angle = 0.0f;
    }
  } else if (strncmp((char*)cmd, "proto_justfloat", 16) == 0) {
    protocol_mode = 0;
  } else if (strncmp((char*)cmd, "proto_firewater", 16) == 0) {
    protocol_mode = 1;
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
