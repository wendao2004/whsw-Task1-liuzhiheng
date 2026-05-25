#include "led.h"
#include "main.h"

static uint8_t blink_mode = 0;
static uint32_t last_toggle = 0;

void led_on(void) {
  blink_mode = 0;
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_SET);
}

void led_off(void) {
  blink_mode = 0;
  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);
}

void led_blink(void) {
  blink_mode = !blink_mode;
}

void led_update(void) {
  if (blink_mode) {
    if (HAL_GetTick() - last_toggle >= 100) {
      last_toggle = HAL_GetTick();
      HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);
    }
  }
}