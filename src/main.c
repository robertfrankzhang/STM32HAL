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
#include <stm32_hal_legacy.h>
#include "pwm.h"
#include "definitions.h"
			
void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull);
void initAllPins(void);

TIM_HandleTypeDef _htim;
TIM_HandleTypeDef _htim2;

int main(void){
	HAL_Init();
	//__HAL_RCC_GPIOA_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();

	initAllPins();

	uint8_t isDispensing = 0;

	while(1){
		if (!HAL_GPIO_ReadPin(topButton) && !HAL_GPIO_ReadPin(bottomButton)){
			__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 200);
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

//		if (photoresistor){
//			__HAL_TIM_SET_COMPARE(&_htim, TIM_CHANNEL_1, 10);
//			HAL_GPIO_WritePin(proxLED,GPIO_PIN_RESET);

//		}
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
	pwm_Init(GPIO_PIN_6,TIM3,GPIOA,40000,200,10,TIM_CHANNEL_1,&_htim);

	//Proximity LED
	initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_5,GPIO_NOPULL);

	//Photoresistor
//	initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_4,GPIO_NOPULL);
}
