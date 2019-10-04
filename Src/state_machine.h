#include <stdint.h>
#include "stm32f1xx.h"

#ifndef __state_machine
#define __state_machine
void state_machine_run(void);

enum CurrentState{
  IDLE,ABLE_TO_DISPENSE,DISPENSING,DOWNLOADING
};

enum SleepLevel {
  SleepLevel_Wake,
  SleepLevel_WaitEvent,
  SleepLevel_Delay,
  SleepLevel_DeepSleep,
};
extern enum SleepLevel sleepLevel;
void goSleep(enum SleepLevel level);

extern enum CurrentState state;
extern uint32_t batValue;
#endif //__state_machine
