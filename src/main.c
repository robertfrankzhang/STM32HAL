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
#define USE_INTERRUPT
			
void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull);
void initAllPins(void);
void SystemClock_Config(void);
void setNextAlarm(void);
static void MX_GPIO_Init(void);

TIM_HandleTypeDef _htim;
TIM_HandleTypeDef _htim2;
ADC_HandleTypeDef hadc1;
static void MX_ADC1_Init(void);

enum DispenseState{
	IDLE,WAITING_FOR_DISPENSE,IS_DISPENSING
};

enum DispenseState state = WAITING_FOR_DISPENSE;
uint8_t buttonDown = 0; //state of whether button is pressed down

int main(void){
	HAL_Init();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	MX_ADC1_Init();
	/* Configure the system clock */
	SystemClock_Config();
	/* Initialize all configured peripherals */
	RTC_Init();
	MX_GPIO_Init();
	initAllPins();


	uint32_t value=0;

	while(1){
		if (!HAL_GPIO_ReadPin(button)){
			if (state == WAITING_FOR_DISPENSE && buttonDown == 0){
				//make timestamp

				HAL_GPIO_WritePin(motor,GPIO_PIN_SET);
				HAL_GPIO_WritePin(proxLED,GPIO_PIN_SET);
				state = IS_DISPENSING;
				buttonDown = 1;
				HAL_Delay(50);
			}
		}else{
			if (buttonDown == 1){
				buttonDown = 0;
			}
		}

		if (state == IS_DISPENSING){
			if (HAL_GPIO_ReadPin(tiltSwitch)){
				HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);
			}else{
				HAL_GPIO_WritePin(motor,GPIO_PIN_SET);
				HAL_Delay(50);
			}

			HAL_ADC_Start(&hadc1);
			if (HAL_ADC_PollForConversion(&hadc1, 1000000) == HAL_OK){
				value = HAL_ADC_GetValue(&hadc1);
				if (value > 1000){
					HAL_GPIO_WritePin(proxLED,GPIO_PIN_RESET);
					HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);

					state = IDLE;
					__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 0);
					//Start a lock out clock for can Dispense;
					setNextAlarm();
				}
			}
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
	//Motor Output
	initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_10,GPIO_NOPULL);

	//Button
#ifdef USE_INTERRUPT
	initPin(GPIOA,GPIO_MODE_IT_FALLING,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_9,GPIO_PULLUP);
#else
	initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_9,GPIO_PULLUP);
#endif
	//Tilt Switch
	initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_11,GPIO_PULLUP);

	//Pulsing Visible LED
	pwm_Init(GPIO_PIN_6,TIM3,GPIOA,40000,400,2,TIM_CHANNEL_1,&_htim);

	//Proximity LED
	initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_5,GPIO_NOPULL);

	//Photoresistor
//	initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_4,GPIO_NOPULL);
}

void deepSleep(){
	//Sleep
	HAL_SuspendTick();
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);

	//Wake Up
	SystemClock_Config();
	HAL_ResumeTick();
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

void setNextAlarm(void){
	RTC_AlarmTypeDef alarm;
	RTC_TimeTypeDef *time = &alarm.AlarmTime;
	HAL_RTC_GetTime(&hrtc, time,  RTC_FORMAT_BIN);

	time->Seconds += 15;

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
  state = WAITING_FOR_DISPENSE;
  __HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 2);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  //__HAL_RCC_GPIOC_CLK_ENABLE();
  //__HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

void EXTI9_5_IRQHandler(void){
	HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
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

