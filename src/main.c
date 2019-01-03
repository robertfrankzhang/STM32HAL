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
			
void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull);
void initAllPins(void);

TIM_HandleTypeDef _htim;
TIM_HandleTypeDef _htim2;
ADC_HandleTypeDef hadc1;
static void MX_ADC1_Init(void);


int main(void){
	HAL_Init();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	MX_ADC1_Init();

	initAllPins();

	uint8_t isDispensing = 0;
	uint32_t value=0;

	while(1){
		if (!HAL_GPIO_ReadPin(topButton) && !HAL_GPIO_ReadPin(bottomButton)){
			//__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 400);
			HAL_GPIO_WritePin(motor,GPIO_PIN_SET);
			HAL_GPIO_WritePin(proxLED,GPIO_PIN_SET);
			isDispensing = 1;
		}

		if (HAL_GPIO_ReadPin(tiltSwitch) && isDispensing == 1){
			HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);
		}else{
			if (isDispensing == 1){
				HAL_GPIO_WritePin(motor,GPIO_PIN_SET);
			}
		}

		HAL_ADC_Start(&hadc1);
		if (HAL_ADC_PollForConversion(&hadc1, 1000000) == HAL_OK){
			value = HAL_ADC_GetValue(&hadc1);
			if (value > 1000 && isDispensing == 1){
				//__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 2);
				HAL_GPIO_WritePin(proxLED,GPIO_PIN_RESET);
				HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);
				isDispensing = 0;
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

	//Top Button
	initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_9,GPIO_PULLUP);

	//Bottom Button
	initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_12,GPIO_PULLUP);

	//Tilt Switch
	initPin(GPIOA,GPIO_MODE_INPUT,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_11,GPIO_PULLUP);

	//Pulsing Visible LED
	pwm_Init(GPIO_PIN_6,TIM3,GPIOA,40000,400,2,TIM_CHANNEL_1,&_htim);

	//Proximity LED
	initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_5,GPIO_NOPULL);

	//Photoresistor
//	initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_4,GPIO_NOPULL);
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

