#include <stdint.h>
#include "stm32f1xx.h"

#ifndef __state_machine
#define __state_machine
void state_machine_run(void);
void deepSleep(void);
enum CurrentState{
  DEAD,IDLE,ABLE_TO_DISPENSE,DISPENSING,DOWNLOADING,DOCKED,UPLOADING,UNLOCKED
};
extern enum CurrentState state;
extern uint32_t batValue;
extern uint32_t shouldDeepSleep;
#endif //__state_machine
