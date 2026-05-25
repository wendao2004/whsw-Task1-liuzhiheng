#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

void motor_on(void);
void motor_off(void);
void motor_set_speed(uint16_t speed);
void motor_update(void);
float motor_get_speed(void);

#endif