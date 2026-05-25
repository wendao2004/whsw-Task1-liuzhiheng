#include "data_send.h"
#include "usbd_cdc_if.h"
#include <string.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t send_mode = 0;
static float run_time = 0.0f;

void data_set_mode(uint8_t mode) {
  send_mode = mode;
}

void data_send_float(float ch0, float ch1) {
  if (send_mode == 0) {
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
    uint32_t hours = (uint32_t)(run_time / 3600);
    uint32_t minutes = (uint32_t)(run_time / 60) % 60;
    uint32_t seconds = (uint32_t)run_time % 60;
    
    uint16_t len = sprintf(buffer, "2026-05-19 %02lu:%02lu:%02lu %.2f\n",
                          hours, minutes, seconds, run_time);
    CDC_Transmit_FS((uint8_t*)buffer, len);
  }
}

float data_get_time(void) {
  return run_time;
}

void data_update_time(float seconds) {
  run_time = seconds;
}