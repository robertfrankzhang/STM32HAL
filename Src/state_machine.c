
#include "stm32f1xx.h"
#include "pwm.h"
#include "definitions.h"
#include "db.h"
#include "serial.h"
#include "dockingProcess.h"
#include "state_machine.h"
#include "main.h"
#include "hardware_abstraction_layer.h"

void deepSleep(void);
void MX_ADC_Init_ex();

extern TIM_HandleTypeDef htim3;
extern ADC_HandleTypeDef hadc1;
extern RTC_HandleTypeDef hrtc;

enum CurrentState state = ABLE_TO_DISPENSE;

uint8_t  EventAlarm = 0;//For lockout period, and LED pulse
uint8_t hasTimeoutEnded = 1; //1 if yes, no if no.
uint8_t  EventPush = 0;
uint8_t EventDock = 0;
uint8_t EventPluggedIn = 0;

uint32_t batValue=0;

typedef enum {
	SAMPLE_BG,
	SAMPLE_IR
} SampleState;

SampleState sample = SAMPLE_BG;
int32_t BG_ADC_Value = 0;
int32_t IR_ADC_Value = 0;

void state_machine_run(void){
  batValue = getADC(batteryVoltageADC);

  switch(state){
  case DEAD://For when battery is too low
	  //Turn off Dispense LED
	  HAL_GPIO_WritePin(dispenseLED,GPIO_PIN_RESET);

	  //Handle if timeout occurs while in DEAD state. No state change until plugged in
	  if (EventAlarm){
		  EventAlarm = 0;
		  hasTimeoutEnded = 1;
	  }

	  //Handle if plugged in while in DEAD state
	  if (EventPluggedIn && prescriptionData.pillCount > 0){//EventPluggedIn toggled when plugged in interrupt occurs
		  if (hasTimeoutEnded){
			  state = ABLE_TO_DISPENSE;
			  setAlarm(1); //Force to be woken up one more time to start ABLE_TO_DISPENSE
		  }else{
			  state = IDLE;
		  }
	  }
    break;

  case IDLE://For when not able to dispense yet
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
	  break;
  case ABLE_TO_DISPENSE://For when able to dispense, but haven't pressed
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
		  if( HAL_GPIO_ReadPin(userSwitch)==0){
			  DB_add(Event_ALLOWED);
			  state = DISPENSING;
		  }
	  }
	  break;
  case DISPENSING://For when the device is dispensing
	  //Set up Timer 2 as internal timer for IR rapid interrupt
	  htim2.Init.Prescaler = 48;
	  htim2.Init.Period = 500;
	  HAL_TIM_Base_Init(&htim2);
	  HAL_TIM_Base_Start_IT(&htim2);
	  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	  HAL_NVIC_EnableIRQ(TIM2_IRQn);
	  //Dispensing Logi

	  setAlarm(prescriptionData.lockoutPeriod);
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
    state = DOWNLOADING
    __HAL_GPIO_EXTI_CLEAR_IT(isPluggedInPin);
  }

  HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
}

void EXTI15_10_IRQHandler(void){
	 uint32_t pending = EXTI->PR;
	 if (pending & (1<<13) && (state == IDLE || state == ABLE_TO_DISPENSE)){
		 EventPush = 1;
		 __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
	 }
	 HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (sample == SAMPLE_BG){
		BG_ADC_Value = getADC(irReceiverADC);
		sample = SAMPLE_IR;
		HAL_GPIO_WritePin(IRLED,SET);
	}else if (sample == SAMPLE_IR){
		IR_ADC_Value = getADC(irReceiverADC);
		sample = SAMPLE_BG;
		HAL_GPIO_WritePin(IRLED,RESET);
		if (IR_ADC_Value-BG_ADC_Value > 30){//Perhaps make it offset + threshold value, where offset = a fixed value/device that = light-bg with nothing there
			HAL_GPIO_WritePin(IRLED,SET);
			//Stop TIMER, stop INTERRUPT, stop DISPENSE, set ALARM, change STATE
		}else{
			HAL_GPIO_WritePin(IRLED,RESET);
		}
	}

}

void deepSleep(void){
  //Sleep
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
