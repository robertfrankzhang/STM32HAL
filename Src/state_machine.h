#include <stdint.h>
#include "stm32f1xx.h"

#ifndef __state_machine
#define __state_machine
void state_machine_init(void);
void state_machine_run(void);
void deepSleep(void);
enum DispenseState{
  IDLE,WAITING_FOR_DISPENSE,IS_DISPENSING,DEAD
};
void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull);
extern enum DispenseState state;
#endif //__state_machine
