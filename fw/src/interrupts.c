#include "stm32f030x6.h"
#include "pins.h"
#include "tb6612.h"

void EXTI4_15_IRQ_handler(void) {
  /* Select EXTI line, which was triggered and call its handler */
  if (EXTI->PR & (1 << PIN_IRQA)) {
    EXTI->PR |= (1 << PIN_IRQA);
    /* Call handler */
    Set_TB6612_Dir(MOTOR_A, DIR_STOP, 0);
  }
  if (EXTI->PR & (1 << PIN_IRQB)) {
    EXTI->PR |= (1 << PIN_IRQB);
    /* Call handler */
    Set_TB6612_Dir(MOTOR_B, DIR_STOP, 0);
  }
}