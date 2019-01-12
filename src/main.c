/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f1xx.h"
#include "pwm.h"
#include "definitions.h"
#include "rtc.h"
#include "db.h"
			
void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull);
void initAllPins(void);
void SystemClock_Config();
void setNextAlarm(int delay);

TIM_HandleTypeDef _htim;
TIM_HandleTypeDef _htim2;
ADC_HandleTypeDef hadc1;
static void MX_ADC1_Init(void);

enum DispenseState{
	IDLE,WAITING_FOR_DISPENSE,IS_DISPENSING
};

enum BufferState{
	CONNECTED,DISCONNECTED,NONE
};

enum DispenseState state = WAITING_FOR_DISPENSE;

uint8_t  EventAlarm = 0;
uint8_t  EventPush = 0;


int main(void){
	HAL_Init();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	MX_ADC1_Init();
	/* Configure the system clock */
	SystemClock_Config();
	/* Initialize all configured peripherals */
	RTC_Init();
	initAllPins();

	//HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
	//__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	setNextAlarm(2);

	while(1){
		deepSleep();
		switch(state){
		case IDLE:
			if (EventAlarm){
				EventAlarm = 0;
				state = WAITING_FOR_DISPENSE;
				//__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 2);
				setNextAlarm(2);
			}
			if (EventPush){
				EventPush = 0;
				DB_add(Event_IDLE);
			}
			break;
		case WAITING_FOR_DISPENSE:
			if (EventAlarm){
				EventAlarm = 0;
				state = WAITING_FOR_DISPENSE;
				//__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 2);
				initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_6,GPIO_NOPULL);
				HAL_GPIO_WritePin(pulseLED,GPIO_PIN_SET);
				HAL_Delay(10);
				HAL_GPIO_WritePin(pulseLED,GPIO_PIN_RESET);
				setNextAlarm(2);
			}
			if (EventPush){
				EventPush = 0;
				if( HAL_GPIO_ReadPin(button)==0){
					DB_add(Event_ALLOWED);
					state = IS_DISPENSING;
					pwm_Init(GPIO_PIN_6,TIM3,GPIOA,40000,400,2,TIM_CHANNEL_1,&_htim);
					//__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 2);
					dispensingProc();
					__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 0);
					setNextAlarm(30);
				}
			}
			break;
		}
	}
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
	initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All&~(GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_5|GPIO_PIN_6),GPIO_NOPULL);
	initPin(GPIOB,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
	initPin(GPIOC,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
	initPin(GPIOD,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
	initPin(GPIOE,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);

	//Motor Output
	initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_10,GPIO_NOPULL);

	//Button
	initPin(GPIOA,GPIO_MODE_IT_FALLING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_9,GPIO_PULLUP);

	//Tilt Switch
	initPin(GPIOA,GPIO_MODE_IT_RISING_FALLING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_8,GPIO_PULLUP);

	//Pulsing Visible LED
	if (state == IS_DISPENSING){
		pwm_Init(GPIO_PIN_6,TIM3,GPIOA,40000,400,2,TIM_CHANNEL_1,&_htim);
	}else{
		initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_6,GPIO_NOPULL);
	}

	//Proximity LED
	initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_5,GPIO_NOPULL);

	//Photoresistor
//	initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_4,GPIO_NOPULL);
}

void deepSleep(){

	//Sleep
	HAL_SuspendTick();
	initPin(GPIOB,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
	initPin(GPIOC,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
	initPin(GPIOD,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);
	initPin(GPIOE,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All,GPIO_NOPULL);

	if (state == IDLE || state == WAITING_FOR_DISPENSE){
		initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All&~(GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10),GPIO_NOPULL);
		initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_8,GPIO_PULLDOWN);//motor
		initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_10,GPIO_PULLDOWN);//tilt switch
		//initPin(GPIOA,GPIO_MODE_IT_FALLING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_9,GPIO_PULLUP);//button
	}

	HAL_ADC_DeInit(&hadc1);
	//HAL_TIM_PWM_DeInit(&_htim);

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
	MX_ADC1_Init();
	HAL_ResumeTick();

}

void dispensingProc(void){
	enum BufferState currentBuffer = CONNECTED;
	const uint16_t MAX_COUNTER = 500; //Change to make buffer longer/shorter. Make it 0 to remove.
	uint16_t bufferCounter = MAX_COUNTER;
	uint32_t value=0;
	while (state == IS_DISPENSING){
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
					HAL_Delay(50);
					currentBuffer = NONE;
					bufferCounter = 0;
				}
				HAL_ADC_Start(&hadc1);
				if (HAL_ADC_PollForConversion(&hadc1, 1000000) == HAL_OK){
					value = HAL_ADC_GetValue(&hadc1);
					if (value > 1000){
						HAL_GPIO_WritePin(proxLED,GPIO_PIN_RESET);
						HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);

						state = IDLE;

					}
				}
			}


		}

	}
}

static void MX_ADC1_Init(void)
{

  ADC_ChannelConfTypeDef sConfig = {0};

  __HAL_RCC_ADC1_CLK_ENABLE();

  hadc1.Instance = ADC1;

  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /**Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

void setNextAlarm(int delay){
	RTC_AlarmTypeDef alarm;
	RTC_TimeTypeDef *time = &alarm.AlarmTime;
	HAL_RTC_GetTime(&hrtc, time,  RTC_FORMAT_BIN);

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

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(){
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /**Initializes the CPU, AHB and APB busses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;

  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

