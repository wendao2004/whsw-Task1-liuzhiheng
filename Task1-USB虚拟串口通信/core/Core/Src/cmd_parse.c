#include "cmd_parse.h"
#include "led.h"
#include "motor.h"
#include "data_send.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

extern uint8_t sinx_mode;

void cmd_process(uint8_t *cmd, uint32_t len) {
  if (len > 0 && len < APP_RX_DATA_SIZE) {
    cmd[len] = '\0';
  }
  
  char *p = (char*)cmd;
  while (*p != '\0') {
    if (*p == '\r' || *p == '\n') {
      *p = '\0';
      break;
    }
    p++;
  }
  
  if (strcmp((char*)cmd, "led_on") == 0) {
    led_on();
  } else if (strcmp((char*)cmd, "led_off") == 0) {
    led_off();
  } else if (strcmp((char*)cmd, "led_blink") == 0) {
    led_blink();
  } else if (strcmp((char*)cmd, "sinx") == 0) {
    sinx_mode = !sinx_mode;
  } else if (strcmp((char*)cmd, "proto_justfloat") == 0) {
    data_set_mode(0);
  } else if (strcmp((char*)cmd, "proto_firewater") == 0) {
    data_set_mode(1);
  } else if (strcmp((char*)cmd, "motor_on") == 0) {
    motor_on();
  } else if (strcmp((char*)cmd, "motor_off") == 0) {
    motor_off();
  } else if (strncmp((char*)cmd, "speed:", 6) == 0) {
    uint16_t speed = atoi((char*)cmd + 6);
    motor_set_speed(speed);
  }
}