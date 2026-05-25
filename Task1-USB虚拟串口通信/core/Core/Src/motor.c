#include "motor.h"
#include "main.h"

extern TIM_HandleTypeDef htim4;

static uint8_t motor_state = 0;
static uint16_t motor_speed_val = 0;
static float motor_rpm_val = 0.0f;
static uint32_t pwm_counter = 0;
static uint32_t last_check = 0;
static int32_t last_encoder = 0;

void motor_on(void) {
  motor_state = 1;
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
}

void motor_off(void) {
  motor_state = 0;
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
}

void motor_set_speed(uint16_t speed) {
  if (speed > 1000) speed = 1000;
  motor_speed_val = speed;
}

void motor_update(void) {
  if (motor_state && motor_speed_val > 0) {
    uint32_t high_time = motor_speed_val / 100;
    if (pwm_counter < high_time) {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    }
  }
  pwm_counter++;
  if (pwm_counter >= 10) pwm_counter = 0;

  if (HAL_GetTick() - last_check >= 100) {
    last_check = HAL_GetTick();
    int32_t current = __HAL_TIM_GET_COUNTER(&htim4);
    int32_t diff = current - last_encoder;
    last_encoder = current;
    motor_rpm_val = (float)diff * 6.0f;
  }
}

float motor_get_speed(void) {
  return motor_rpm_val;
}