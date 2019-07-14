
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
enum CurrentState prevState = ABLE_TO_DISPENSE;//Not relevant/doesn't have to be accurate except for plug in mode.

uint8_t EventAlarm = 0;
uint8_t hasTimeoutEnded = 1; //1 if yes, 0 if no.
uint8_t EventPush = 0;
uint8_t EventDock = 0;
uint8_t EventPluggedIn = 0;
uint8_t EventIRSample = 0;

uint32_t batValue=0;
uint32_t shouldDeepSleep = 1;

typedef enum {
	SAMPLE_BG,
	SAMPLE_IR
} SampleState;

SampleState sample = SAMPLE_BG;
int32_t BG_ADC_Value = 0;
int32_t IR_ADC_Value = 0;

void startDownloadProcess(enum CurrentState currentState){//currentState is state to go back to if dock unrecognized
	prevState = currentState;
	state = DOWNLOADING;
	shouldDeepSleep = 0; //Skip sleep cycle once to start DOWNLOADING
}

uint32_t dispenseFailed(){//always returns 0. Sets state back to ABLE_TO_DISPENSE
	state = ABLE_TO_DISPENSE;
	HAL_GPIO_WritePin(motorSleep,RESET);
	_db.item[_db.index].dispenseStatus = Dispense_FAIL;
	setAlarm(2);
	return 0;
}

void state_machine_run(void){
  switch(state){
  case DEAD://For when battery is too low
	  shouldDeepSleep = 1;
	  //Turn off Dispense LED
	  HAL_GPIO_WritePin(dispenseLED,GPIO_PIN_RESET);

	  //Handle if timeout occurs while in DEAD state. No state change until plugged in
	  if (EventAlarm){
		  EventAlarm = 0;
		  hasTimeoutEnded = 1;
	  }

	  //Handle if plugged in while in DEAD state. Sets prevState to be one of 2 other states if there are still pills left
	  if (EventPluggedIn){
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
    break;
  case IDLE://For when not able to dispense yet
	  shouldDeepSleep = 1;
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

	  if (EventPluggedIn){
		  EventPluggedIn = 0;
		  startDownloadProcess(state);
	  }

	  //If battery level is too low
	  if (getADC(batteryVoltageADC)<50){//50 needs to be tested & adjusted
		  state = DEAD;
		  shouldDeepSleep = 0; //Skip sleep cycle once to start DEAD
	  }
	  break;
  case ABLE_TO_DISPENSE://For when able to dispense, but haven't pressed
	  shouldDeepSleep = 1;
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

	  //If Plugged In
	  if (EventPluggedIn){
		  EventPluggedIn = 0;
		  startDownloadProcess(state);
	  }

	  //If battery level is too low
	  if (getADC(batteryVoltageADC)<50){//50 needs to be tested & adjusted
		  state = DEAD;
		  shouldDeepSleep = 0; //Skip sleep cycle once to start DEAD
	  }
	  break;
  case DISPENSING://For when the device is dispensing
	  shouldDeepSleep = 1;
	  //Set up Timer 2 as internal timer for IR rapid interrupt
	  htim2.Init.Prescaler = 48;
	  htim2.Init.Period = 500;
	  HAL_TIM_Base_Init(&htim2);
	  HAL_TIM_Base_Start_IT(&htim2);
	  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	  HAL_NVIC_EnableIRQ(TIM2_IRQn);

	  uint8_t continueDispense = 1;
	  uint8_t jamCounter = 0;
	  uint8_t motorDirection = 1;//odds are forward, evens are backwards
	  //Start motor and motor Alarm
	  HAL_GPIO_WritePin(lockMotorA2,RESET);
	  HAL_GPIO_WritePin(lockMotorA1,SET);//Should be PWM init though
	  HAL_GPIO_WritePin(motorSleep,SET);
	  setAlarm(5);

	  while (continueDispense){
		  //If Dispense Failed
		  if (EventAlarm){
			  EventAlarm = 0;
			  continueDispense = dispenseFailed();
		  }

		  //If Button Pressed
		  if (EventPush){
			  EventPush = 0;//Do Nothing
		  }

		  //NOTE: No event plug in here, because that will be automatically handled upon state change.

		  //If motor fault, dispense failed
		  if (HAL_GPIO_ReadPin(motorFault) == 0){
			  continueDispense = dispenseFailed();
		  }

		  //If Jam Detected
		  if(getADC(motorAFLTADC) > 50){//50 value needs to be tested
			  jamCounter++;
			  if (jamCounter>=4){//Dispense Failed
				  continueDispense = dispenseFailed();
			  }
			  else if (motorDirection%2 == 1){//Switch to Backward Spin
				  motorDirection++;
				  HAL_GPIO_WritePin(lockMotorA2,SET);
				  HAL_GPIO_WritePin(lockMotorA1,RESET);//Should be PWM init though
				  setAlarm(5);
			  }else{//Switch to Forward Spin
				  motorDirection++;
				  HAL_GPIO_WritePin(lockMotorA2,RESET);
				  HAL_GPIO_WritePin(lockMotorA1,SET);//Should be PWM init though
				  setAlarm(5);
			  }
		  }

		  //Dispensing Logic
		  if (EventIRSample){
			  EventIRSample = 0;
			  if (sample == SAMPLE_BG){
				  BG_ADC_Value = getADC(irReceiverADC);
		  		  sample = SAMPLE_IR;
		  		  HAL_GPIO_WritePin(IRLED,SET);
			  }else if (sample == SAMPLE_IR){
		  		  IR_ADC_Value = getADC(irReceiverADC);
		  		  sample = SAMPLE_BG;
		  		  HAL_GPIO_WritePin(IRLED,RESET);
		  		  if (IR_ADC_Value-BG_ADC_Value > 30){//Perhaps make it offset + threshold value, where offset = a fixed value/device that = light-bg with nothing there
		  			  //Stop TIMER, stop INTERRUPT, stop DISPENSE, set ALARM, change STATE
		  			  continueDispense = 0;
		  			  prescriptionData.pillCount--;
		  			  _db.item[_db.index].dispenseStatus = Dispense_SUCCESS;
		  			  HAL_GPIO_WritePin(motorSleep,RESET);
		  			  if (prescriptionData.pillCount <= 0){
		  				  state = DEAD;
		  				  shouldDeepSleep = 0; //Skip sleep cycle once to start DEAD
		  			  }
		  			  else if (prescriptionData.operatingMode == Opmode_ASNEEDED){
		  				  state = ABLE_TO_DISPENSE;
		  				  setAlarm(2);
		  			  }else{
		  				  state = IDLE;
		  				  setAlarm(prescriptionData.lockoutPeriod);
		  			  }
		  		  }
			  }
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
    state = DOWNLOADING;
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
	EventIRSample = 1;
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
