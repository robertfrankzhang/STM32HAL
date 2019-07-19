
#include "definitions.h"

#ifdef HW_TEST // no need all if not defined
#include "hw_test.h"
#include "hardware_abstraction_layer.h"
extern uint8_t EventIRSample;

// forward declaration
void hw_test_motor_on_off();
void hw_test_motor_overload();
void hw_test_ir_read();
void hw_test_battery_read();
void hw_test_tim1();

// for test only
uint32_t test_val[8];

void hw_test(){
  // only select one to test
  int test_case = 4;

  switch(test_case){
  case 0:
    hw_test_motor_on_off();
    break;
  case 1:
    hw_test_motor_overload();
    break;
  case 2:
    hw_test_ir_read();
    break;
  case 3:
    hw_test_battery_read();
    break;
  case 4:
    hw_test_tim1();
    break;
  }
}

// test motor
void hw_test_motor_on_off(){
  while(1){
    // spin dispense Motor
    spinDispenseMotor(1); // reverse
    HAL_Delay(1000);
    spinDispenseMotor(0); // forward
    HAL_Delay(1000);
    stopDispenseMotor(); // stop
    HAL_Delay(1000);
    // spin lock Motor
    spinLockMotor(1);
    HAL_Delay(1000);
    spinLockMotor(0);
    HAL_Delay(1000);
    stopLockMotor();
    HAL_Delay(1000);
  }
}

void hw_test_motor_overload(){
  // hold motor spinning to check valtage
  int8_t direction = 0;
  while(1){
    spinDispenseMotor(direction); // forward
    for(int i=0; i<8; ++i){
      HAL_Delay(100);
      if(dispenseMotorCurrent()>2 || motorIsFault()){
    	  direction ^= 1;
    	  break;
      }
    }
  }
}

void hw_test_ir_read(){
  while(1){
    HAL_GPIO_WritePin(irReceiverPower,SET);
    for(int i=0; i<4; ++i){
      HAL_GPIO_WritePin(IRLED,SET);
      HAL_Delay(10);
      test_val[2*i] = getADC(irReceiverADC);
      HAL_GPIO_WritePin(IRLED,RESET);
      HAL_Delay(10);
      test_val[2*i+1] = getADC(irReceiverADC);
    }
    HAL_GPIO_WritePin(irReceiverPower,RESET);
    HAL_Delay(500);
  }
}

void hw_test_battery_read(){
  while(1){
    for(int i=0; i<8; ++i){
      test_val[i] = getADC(batteryVoltageADC);
      HAL_Delay(250);
    }
    HAL_Delay(100); // set break here
  }
}

void hw_test_tim1(){
  while(1){
    startIRSamplingTimer();
    for(int i=0; i<8; ++i){
      test_val[i] = htim1.Instance->CNT;
      HAL_Delay(10);
    }
    if(EventIRSample){
      EventIRSample = 0;
      HAL_Delay(10);
    }
    HAL_Delay(10);
  }
}

#endif // HW_TEST
