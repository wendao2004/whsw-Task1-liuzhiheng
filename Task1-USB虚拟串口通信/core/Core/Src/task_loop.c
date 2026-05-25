#include "task_loop.h"
#include "led.h"
#include "motor.h"
#include "data_send.h"
#include "main.h"
#include <math.h>
#include <stdint.h>

extern uint8_t sinx_mode;

static uint32_t last_send = 0;

void task_init(void) {
  last_send = HAL_GetTick();
}

void task_run(void) {
  float run_time = (float)HAL_GetTick() / 1000.0f;
  data_update_time(run_time);
  
  motor_update();
  
  led_update();
  
  if (HAL_GetTick() - last_send >= 100) {
    last_send = HAL_GetTick();
    
    float rpm = motor_get_speed();
    data_send_float(rpm, (float)0);
    
    if (sinx_mode) {
      float angle = (float)HAL_GetTick() * 0.001f;
      float sin_value = sinf(angle);
      data_send_float(run_time, sin_value);
    }
  }
}