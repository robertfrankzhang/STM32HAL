/*
 * hardware_abstraction_layer.c
 *
 *  Created on: Jul 13, 2019
 *      Author: robert
 */

#include "stm32f1xx.h"
#include "pwm.h"
#include "definitions.h"
#include "db.h"
#include "serial.h"
#include "dockingProcess.h"
#include "state_machine.h"
#include "main.h"




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

void setAlarm(int delay){
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

void setPinsOfPortToLowPower(GPIO_TypeDef* port, uint32_t pin1, uint32_t pin2, uint32_t pin3){
	initPin(port,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_All&~(pin1|pin2|pin3),GPIO_NOPULL);
}

void disableClocks(){
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
}

void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull){
  GPIO_InitTypeDef gpio;
  gpio.Mode = mode;
  gpio.Speed = speed;
  gpio.Pin = pin;
  gpio.Pull  = pull;
  HAL_GPIO_Init(port, &gpio);
}
