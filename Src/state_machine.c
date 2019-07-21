
#include "stm32f1xx.h"
#include "pwm.h"
#include "definitions.h"
#include "db.h"
#include "serial.h"
#include "dockingProcess.h"
#include "state_machine.h"
#include "main.h"
#include "hardware_abstraction_layer.h"

void MX_ADC_Init_ex();

//extern TIM_HandleTypeDef htim3;
extern ADC_HandleTypeDef hadc1;
extern RTC_HandleTypeDef hrtc;

enum CurrentState state = ABLE_TO_DISPENSE;
enum CurrentState prevState = ABLE_TO_DISPENSE;//Not relevant/doesn't have to be accurate except for plug in mode.

uint8_t EventAlarm = 0;
uint8_t hasTimeoutEnded = 1; //1 if yes, 0 if no.
uint8_t EventPush = 0;
uint8_t EventDock = 0;
uint8_t EventPluggedIn = 0;
uint8_t EventIRSample = 0;

uint32_t batValue=0;

enum SleepLevel sleepLevel = SleepLevel_Wake;

typedef enum {
  SAMPLE_BG,
  SAMPLE_IR
} SampleState;

SampleState sample = SAMPLE_BG;
int16_t BG_ADC_Value = 0;
int16_t IR_ADC_Value = 0;

void startDownloadProcess(enum CurrentState currentState){//currentState is state to go back to if dock unrecognized
  prevState = currentState;
  state = DOWNLOADING;
  sleepLevel = SleepLevel_Wake; //Skip sleep cycle once to start DOWNLOADING
}


void state_machine_run(void){
  switch(state){
  case DEAD://For when battery is too low
#ifdef SERIAL_DEBUG
    serial_printf("to DEAD\n\r");
#endif
    sleepLevel = SleepLevel_DeepSleep; //Skip sleep cycle once to start DOWNLOADING
          
    //Turn off Dispense LED
    HAL_GPIO_WritePin(dispenseLED,GPIO_PIN_RESET);

    //Handle if timeout occurs while in DEAD state. No state change until plugged in
    if (EventAlarm){
      EventAlarm = 0;
      hasTimeoutEnded = 1;
    }

#ifndef SERIAL_DEBUG
    //Handle if plugged in while in DEAD state. Sets prevState to be one of 2 other states if there are still pills left
    if(usbIsPluggedIn()){
      EventPluggedIn = 0;
      if (prescriptionData.pillCount > 0){
        if (hasTimeoutEnded){
          startDownloadProcess(ABLE_TO_DISPENSE);
        }else{
          startDownloadProcess(IDLE);
        }
      }else{
        startDownloadProcess(state);
      }
    }
#endif
    break;
  case IDLE://For when not able to dispense yet
#ifdef SERIAL_DEBUG
    serial_printf("to IDLE\n\r");
#endif
    sleepLevel = SleepLevel_DeepSleep;
    if (EventAlarm){
      EventAlarm = 0;
      hasTimeoutEnded = 1;
      state = ABLE_TO_DISPENSE;
      setAlarm(2);//Set first LED light flash
    }

    if (EventPush){
      EventPush = 0;
      DB_add(Event_NOTALLOWED);
    }

#ifndef SERIAL_DEBUG
    if(usbIsPluggedIn()){
      EventPluggedIn = 0;
      startDownloadProcess(state);
    }
#endif

    //If battery level is too low
    if (getADC(batteryVoltageADC)<50){//50 needs to be tested & adjusted
      state = DEAD;
      sleepLevel = SleepLevel_Wake; //Skip sleep cycle once to start DEAD
    }
    break;
  case ABLE_TO_DISPENSE://For when able to dispense, but haven't pressed
#ifdef SERIAL_DEBUG
    serial_printf("to ABLE_TO_DISPENSE\n\r");
#endif
    sleepLevel = SleepLevel_DeepSleep;
    hasTimeoutEnded = 0;

    //Handle if 2 second light pulse alarm just passed
    if (EventAlarm){//Flash light
      EventAlarm = 0;
      HAL_GPIO_WritePin(dispenseLED,GPIO_PIN_SET);
      HAL_Delay(2);
      HAL_GPIO_WritePin(dispenseLED,GPIO_PIN_RESET);
      setAlarm(2);
    }

    //Handle if Button was just pressed
    if (EventPush){
      EventPush = 0;
      DB_add(Event_ALLOWED);
      state = DISPENSING;
      sleepLevel = SleepLevel_Wake;
    }

#ifndef SERIAL_DEBUG
    //If Plugged In
    if(usbIsPluggedIn()){
      EventPluggedIn = 0;
      startDownloadProcess(state);
    }
#endif

    //If battery level is too low
    if (getADC(batteryVoltageADC)<50){//50 needs to be tested & adjusted
      state = DEAD;
      sleepLevel = SleepLevel_Wake; //Skip sleep cycle once to start DEAD
    }
    break;
  case DISPENSING://For when the device is dispensing
#ifdef SERIAL_DEBUG
    serial_printf("to DISPENSING\n\r");
#endif
    sleepLevel = SleepLevel_DeepSleep;

    uint8_t continueDispense = 1;
    
    uint8_t  jamCounter = 0;
    uint8_t  motorDirection = 1;//1 is forward, 0 is backwards
    uint32_t motorADCSum;
    uint8_t  motorAvgCnt=0;
    uint32_t motorDelayCnt=0;
    
    uint16_t IR_BG_Threshold = 480;
    uint8_t dispensingFailed = 0;

    uint16_t IR_samlping_delay_cnt=0;
    
    // check if pill is out already
    HAL_GPIO_WritePin(irReceiverPower,SET);//Turn on power for pill drop ADC reader
    HAL_Delay(1);
    IR_ADC_Value = getADC(irReceiverADC);
    HAL_GPIO_WritePin(irReceiverPower,RESET);//Turn off power for pill drop ADC reader
    HAL_Delay(1);
    BG_ADC_Value = getADC(irReceiverADC);
    if( IR_ADC_Value - BG_ADC_Value > IR_BG_Threshold){
      continueDispense = 0;
      dispensingFailed = 1; // previous pill still there, fail for this
    }
    else{
      //Start motor and motor Alarm
      spinDispenseMotor(motorDirection);
      motorDelayCnt = 0;
      continueDispense = 1;
      setAlarm(5);
    }

    HAL_GPIO_WritePin(irReceiverPower,SET);//Turn on power for pill drop ADC reader
    
    // 2 ADC about 28.4khz loop
    while (continueDispense){
      //If Dispense Failed
      if (EventAlarm){
        EventAlarm = 0;
        continueDispense = 0;
        dispensingFailed = 1; // previous pill still there, fail for this
        //continueDispense = dispenseFailed();
        break; // break while loop
      }

      //If Button Pressed
      if (EventPush)
        EventPush = 0;//Do Nothing

      //NOTE: No event plug in here, because that will be automatically handled upon state change.

      //If motor fault, dispense failed
      if (motorIsFault()){
        continueDispense = 0;
        dispensingFailed = 1; // previous pill still there, fail for this
        //continueDispense = dispenseFailed();
        break; // break while loop
      }

      //If Jam Detected
      if(++motorAvgCnt < 100) // number of average samples
        motorADCSum += getADC(motorAFLTADC);
      else{
        motorADCSum /= motorAvgCnt;

        // motoeDelayCnt to avoid initial start current
        if((++motorDelayCnt > 100)){
          if((motorADCSum > 10)) {
            jamCounter++;
            if (jamCounter>=4){//Dispense Failed
              continueDispense = 0;
              dispensingFailed = 1; // previous pill still there, fail for this
              //continueDispense = dispenseFailed();
              break;
            }
            else {
              motorDirection ^= 1;//Switch to Backward Spin
              spinDispenseMotor(motorDirection);
              motorDelayCnt = 0;
              setAlarm(5);
            }
          }
        }

        motorAvgCnt = 0;
        motorADCSum = 0;
      }

      //Dispensing Logic
      if (sample == SAMPLE_BG){
        BG_ADC_Value = getADC(irReceiverADC);
        sample = SAMPLE_IR;
        HAL_GPIO_WritePin(IRLED,SET);
      }else if (sample == SAMPLE_IR){
        IR_ADC_Value = getADC(irReceiverADC);
        sample = SAMPLE_BG;
        HAL_GPIO_WritePin(IRLED,RESET);
      } // if sample

      if (++IR_samlping_delay_cnt > 10){
        IR_samlping_delay_cnt = 0;
        // Perhaps make it offset + threshold value,
        // where offset = a fixed value/device that = light-bg with nothing there
        if((sample == SAMPLE_BG) && (IR_ADC_Value-BG_ADC_Value > IR_BG_Threshold)){
          continueDispense = 0;
          dispensingFailed = 0; // success
          break;
        }
      }// if IR_samlping_delay_cnt
    } // while continueDispensing

    //Stop TIMER, stop DISPENSE, set ALARM, change STATE
    stopDispenseMotor();
    HAL_GPIO_WritePin(irReceiverPower,RESET);//Turn off power for pill drop ADC reader
    HAL_GPIO_WritePin(IRLED,RESET); // turn off IR

    // update db
    if(dispensingFailed){
      state = ABLE_TO_DISPENSE;
      _db.item[_db.index].dispenseStatus = Dispense_FAIL;
      setAlarm(2);
    }
    else {
      prescriptionData.pillCount--;
      _db.item[_db.index].dispenseStatus = Dispense_SUCCESS;
      
      if (prescriptionData.pillCount <= 0){
        state = DEAD;
        sleepLevel = SleepLevel_Wake; //Skip sleep cycle once to start DEAD
      }
      else if (prescriptionData.operatingMode == Opmode_ASNEEDED){
        state = ABLE_TO_DISPENSE;
        setAlarm(2);
      }else{
        state = IDLE;
        setAlarm(prescriptionData.lockoutPeriod);
      }
    }
    break;
  case DOWNLOADING://For authenticating dock and then when the device is downloading code
    break;
  case DOCKED://For when the device is at Dock
    break;
  case UPLOADING://For accepting new prescription data
    break;
  case UNLOCKED://For when unlocked and unplugged

    break;
  default:
    break;
  }
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *rtc) {//Alarm jolts out of deep sleep
  EventAlarm  = 1;
}

void EXTI9_5_IRQHandler(void){//WHEN PINS CHANGE, THESE NUMBERS MUST CHANGE. JUST SWAP 8 for PIN #. Port doesn't matter. THIS FUNC HANGLES PINS 5-9
  uint32_t pending = EXTI->PR;
  if (pending & (1<<8)){
    EventPluggedIn = 1;
    __HAL_GPIO_EXTI_CLEAR_IT(isPluggedInPin);
  }

  HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
}

void EXTI15_10_IRQHandler(void){
  uint32_t pending = EXTI->PR;
  if (pending & (1<<13) && (state == IDLE || state == ABLE_TO_DISPENSE)){
    EventPush = 1;
    //__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
  }
  __HAL_GPIO_EXTI_CLEAR_IT(pending);
  HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  EventIRSample = 1;
}

void deepSleep(){
  HAL_SuspendTick();

  //Set all pins to analog, no pull
  setPinsOfPortToLowPower(GPIOA,0,0,0);
  setPinsOfPortToLowPower(GPIOB,userSwitchPin,lockButtonPin,0);
  setPinsOfPortToLowPower(GPIOC,0,0,0);
  setPinsOfPortToLowPower(GPIOD,0,0,0);
  setPinsOfPortToLowPower(GPIOE,0,0,0);

  //Pull down button pins
  initPin(userSwitchPort,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,userSwitchPin,GPIO_PULLDOWN);
  initPin(lockButtonPort,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_MEDIUM,lockButtonPin,GPIO_PULLDOWN);

  //Switch off all ADCs
  HAL_ADC_DeInit(&hadc1);

  //Disable PWMs
  //HAL_TIM_PWM_DeInit(&htim3);

  //Disable Clocks
  disableClocks();

  //Go to deep sleep mode
  //HAL_PWR_EnterSTANDBYMode();
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);

  //Wake Up
  SystemClock_Config();
  initAllPins(); //Calls the MX generated GPIO init function from main
  MX_RTC_Init_ex();
  MX_ADC_Init_ex();
  HAL_ResumeTick();
}

void goSleep(enum SleepLevel level){
  switch(level){
  case SleepLevel_Wake:
    break;
  case SleepLevel_DeepSleep:
#ifndef SERIAL_DEBUG
    deepSleep();
    break;
#endif
  case SleepLevel_WaitEvent:
    while(!EventAlarm && !hasTimeoutEnded && !EventPush &&
          !EventDock && !EventPluggedIn && !EventIRSample )
      HAL_Delay(20);
    break;
  case SleepLevel_Delay:
    HAL_Delay(500);
    break;
  }
}
