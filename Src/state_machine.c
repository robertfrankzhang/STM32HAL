
#include "stm32f1xx.h"
#include "pwm.h"
#include "definitions.h"
#include "db.h"
#include "serial.h"
#include "dockingProcess.h"
#include "state_machine.h"

void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull);
void initAllPins(void);
void setNextAlarm(int delay);
void dispensingProc(void);
void deepSleep(void);
void MX_ADC_Init_ex();
uint32_t getADC(uint32_t channel);

extern TIM_HandleTypeDef htim3;
extern ADC_HandleTypeDef hadc1;
extern RTC_HandleTypeDef hrtc;

enum BufferState{
  CONNECTED,DISCONNECTED,NONE
};

enum DispenseState state = WAITING_FOR_DISPENSE;

uint8_t  EventAlarm = 0;
uint8_t  EventPush = 0;
uint8_t EventDock = 0;

uint32_t batValue=0;

void state_machine_init(void){
  initAllPins();
}

void state_machine_run(void){
  if (EventDock){
	  EventDock = 0;
	  dockingProc();
  }
  batValue = getADC(ADC_CHANNEL_3);

  switch(state){
  case IDLE:
    if (EventAlarm){
      EventAlarm = 0;
      state = WAITING_FOR_DISPENSE;
      //__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 2);
      setNextAlarm(2);//3 second delay (adds 1 always)
    }
    if (EventPush){
      EventPush = 0;
      serial_printf("dbg EvPush idle\n\r");
      DB_add(Event_IDLE);
    }
    break;
  case WAITING_FOR_DISPENSE:
    if (EventAlarm){
      setNextAlarm(2);
      EventAlarm = 0;
      serial_printf("dbg EvAlarm w4d\n\r");
      state = WAITING_FOR_DISPENSE;
      //__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 2);
      initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_6,GPIO_NOPULL);
      HAL_GPIO_WritePin(pulseLED,GPIO_PIN_SET);
      HAL_Delay(2);
      HAL_GPIO_WritePin(pulseLED,GPIO_PIN_RESET);


    }
    if (EventPush){
      serial_printf("dbg EvPush w4d\n\r");
      EventPush = 0;
      if( HAL_GPIO_ReadPin(button)==0){
        DB_add(Event_ALLOWED);
    	state = IS_DISPENSING;
    	pwm_Init(GPIO_PIN_6,TIM3,GPIOA,40000,3000,2,TIM_CHANNEL_1,&htim3);
    	//__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 2);
    	dispensingProc();
    	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    	setNextAlarm(prescriptionData.lockoutPeriod);
      }
    }
    break;
  default:
    break;
  }
  if (prescriptionData.pillCount == 0){
 	  //Turn all things off
 	  HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);
 	  HAL_GPIO_WritePin(proxLED,GPIO_PIN_RESET);
 	  initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_6,GPIO_NOPULL);
 	  HAL_GPIO_WritePin(pulseLED,GPIO_PIN_RESET);
 	  state = DEAD;
   }
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *rtc) {
  EventAlarm  = 1;
}

void EXTI9_5_IRQHandler(void){
  uint32_t pending = EXTI->PR;

  if (pending & (1<<9)){
    EventPush = 1;
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
  }
  if (pending & (1<<8)){
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);
  }
  HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
}

void EXTI15_10_IRQHandler(void){
	 uint32_t pending = EXTI->PR;
	 if (pending & (1<<15)){
		 EventDock = 1;
		 __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
	 }
	 HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
}

void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull){
  GPIO_InitTypeDef gpio;
  gpio.Mode = mode;
  gpio.Speed = speed;
  gpio.Pin = pin;
  gpio.Pull  = pull;
  HAL_GPIO_Init(port, &gpio);
}


void initAllPins(){
  initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All &
		  ~(GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_2),GPIO_NOPULL);
  initPin(GPIOB,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
  initPin(GPIOC,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
  initPin(GPIOD,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
  initPin(GPIOE,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);

  //Motor Output
  initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_10,GPIO_NOPULL);

  //Button
  initPin(GPIOA,GPIO_MODE_IT_FALLING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_9,GPIO_PULLUP);

  //Tilt Switch
  initPin(GPIOA,GPIO_MODE_IT_FALLING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_8,GPIO_PULLUP);

  //Pulsing Visible LED
  if (state == IDLE || state == WAITING_FOR_DISPENSE){
  	initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All &
  			~(GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_2),GPIO_NOPULL);
  	initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_8,GPIO_PULLDOWN);//tilt switch
  	initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_10,GPIO_PULLDOWN);//motor
  	//initPin(GPIOA,GPIO_MODE_IT_FALLING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_9,GPIO_PULLUP);//button
  }

  //Proximity LED
  initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_5,GPIO_NOPULL);

  //Photoresistor
  //	initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_4,GPIO_NOPULL);

  //Charging Detection GPIO
  initPin(GPIOA,GPIO_MODE_IT_RISING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_15,GPIO_NOPULL);

  //Fully Charged Detection
  initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_2,GPIO_PULLUP);

}

void  dispensingProc(void){
  enum BufferState currentBuffer = CONNECTED;
  const uint16_t MAX_COUNTER = 500; //Change to make buffer longer/shorter. Make it 0 to remove.
  uint16_t bufferCounter = MAX_COUNTER;
  uint32_t value=0;
  while (state == IS_DISPENSING){
	if (EventAlarm){
		EventAlarm = 0;
	}
	if (EventDock){
		EventDock = 0;
		dockingProc();
		return;
	}
    if (HAL_GPIO_ReadPin(tiltSwitch)){//If Tilt Switch Disconnected
      if (currentBuffer == CONNECTED || currentBuffer == NONE){
        bufferCounter = 0;
        currentBuffer = DISCONNECTED;
      }
      if (bufferCounter < MAX_COUNTER){
        HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);
        ++bufferCounter;
        HAL_Delay(1);
      }else{
	    if (currentBuffer != NONE){
          HAL_GPIO_WritePin(proxLED,GPIO_PIN_RESET);
          currentBuffer = NONE;
          bufferCounter = 0;
          deepSleep();
	    }
      }
    }else{ //If Tilt Switch Connected
       if (currentBuffer == DISCONNECTED || currentBuffer == NONE){
          bufferCounter = 0;
          currentBuffer = CONNECTED;
      }
      if (bufferCounter < MAX_COUNTER){
	    ++bufferCounter;
	    HAL_Delay(1);
      }else{
        if (currentBuffer != NONE){
          HAL_GPIO_WritePin(motor,GPIO_PIN_SET);
          HAL_GPIO_WritePin(proxLED,GPIO_PIN_SET);
          initPin(GPIOA,GPIO_MODE_IT_FALLING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_8,GPIO_PULLUP);
          HAL_Delay(50);
          currentBuffer = NONE;
          bufferCounter = 0;
        }
      }
      if (HAL_GPIO_ReadPin(proxLED)){
	value = getADC(ADC_CHANNEL_4);
	if (value != 0xffffffff && value > 1000){
	  HAL_GPIO_WritePin(proxLED,GPIO_PIN_RESET);
	  HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);
	  state = IDLE;
	  --prescriptionData.pillCount;
	} // if value
      }
      // if bufferCounter
    } // if tilt switch
  } // while
}

uint32_t getADC(uint32_t channel){
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    return 0xffffffff;
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 1000000) != HAL_OK)
    return 0xffffffff;
  return HAL_ADC_GetValue(&hadc1);
}

void setNextAlarm(int delay){
  RTC_AlarmTypeDef alarm;
  RTC_TimeTypeDef *time = &alarm.AlarmTime;
  HAL_RTC_GetTime(&hrtc, time,  RTC_FORMAT_BIN);

  serial_printf("dbg setAlarm %d\r\n",delay);
  time->Seconds += delay;

  while(time->Seconds>59){
    time->Seconds -= 60;
    ++time->Minutes;
  }
  while(time->Minutes>59){
    time->Minutes -= 60;
    ++time->Hours;
  }
  while(time->Hours>23)
    time->Hours -= 24;

  HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN);
}

#if 0
void deepSleep(void){
  while(1){
	HAL_Delay(20);
	if(EventAlarm||EventPush)
	  initAllPins();
	  return;
  }
}
#else
void deepSleep(void){
  //Sleep
  HAL_SuspendTick();
  initPin(GPIOB,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
  initPin(GPIOC,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
  initPin(GPIOD,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
  initPin(GPIOE,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);

  if (state == IDLE || state == WAITING_FOR_DISPENSE){
    initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All&~(GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10),GPIO_NOPULL);
    initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_8,GPIO_PULLDOWN);
    initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_10,GPIO_PULLDOWN);
  }

  HAL_ADC_DeInit(&hadc1);
  //HAL_TIM_PWM_DeInit(&htim3);

  __HAL_RCC_TIM2_CLK_DISABLE();
  __HAL_RCC_WWDG_CLK_DISABLE();
  __HAL_RCC_USART2_CLK_DISABLE();
  __HAL_RCC_I2C1_CLK_DISABLE();
  __HAL_RCC_BKP_CLK_DISABLE() ;
  __HAL_RCC_PWR_CLK_DISABLE();
  __HAL_RCC_AFIO_CLK_DISABLE();
  __HAL_RCC_GPIOB_CLK_DISABLE();
  __HAL_RCC_GPIOC_CLK_DISABLE();
  __HAL_RCC_GPIOD_CLK_DISABLE();
  __HAL_RCC_ADC1_CLK_DISABLE();
  __HAL_RCC_TIM1_CLK_DISABLE();
  __HAL_RCC_SPI1_CLK_DISABLE() ;
  __HAL_RCC_USART1_CLK_DISABLE();

  //HAL_PWR_EnterSTANDBYMode();
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);

  //Wake Up
  SystemClock_Config();
  initAllPins();
  MX_RTC_Init_ex();
  MX_ADC_Init_ex();
  HAL_ResumeTick();
}
#endif
