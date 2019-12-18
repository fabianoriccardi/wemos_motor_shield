#ifndef __TB6612_H
#define __TB6612_H

#include <stdint.h>
#include "pins.h"

#define MOTOR_A                 0
#define MOTOR_B                 1
#define MOTOR_AB                2

#define DIR_BRAKE               0x00
#define DIR_CCW                 0x01
#define DIR_CW                  0x02
#define DIR_STOP                0x03
#define DIR_STANDBY             0x04

#define PWM_STEPS       256
#define PWM_MAX_FREQ    31250
#define MAX_PERCENTAGE  (100 * 100)

extern void Set_Freq(uint32_t freq);
extern uint32_t Get_Freq(void);
extern void Set_TB6612_Dir(uint8_t motor, uint8_t dir, uint8_t pulse);
extern uint8_t Get_TB6612_State(uint8_t *buf, uint8_t size);

#endif