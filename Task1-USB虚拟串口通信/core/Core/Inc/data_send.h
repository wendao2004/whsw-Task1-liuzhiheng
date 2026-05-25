#ifndef DATA_SEND_H
#define DATA_SEND_H

#include <stdint.h>

void data_set_mode(uint8_t mode);
void data_send_float(float ch0, float ch1);
float data_get_time(void);
void data_update_time(float seconds);

#endif