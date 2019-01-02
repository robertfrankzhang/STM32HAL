/*
 * pwm.c
 *
 *  Created on: Jan 1, 2019
 *      Author: robert
 */

#include "stm32f1xx.h"
#include <stm32_hal_legacy.h>
#include "pwm.h"

void configChannel(TIM_HandleTypeDef* htim_pwm,uint32_t pulse,uint32_t channel){
	TIM_OC_InitTypeDef sConfigOC;
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_SET;
	sConfigOC.Pulse = pulse;  // = 210/2  50% duty cycle
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;

	HAL_TIM_PWM_ConfigChannel(htim_pwm, &sConfigOC, channel);
	HAL_TIM_PWM_Start(htim_pwm,channel);
}

/* TIM init function */
void TIM_Init(uint32_t prescaler, uint32_t period, uint32_t pulse, uint32_t channel,TIM_HandleTypeDef* htim_pwm, uint32_t pin, TIM_TypeDef* timer,GPIO_TypeDef* port) {
  // use t2c2
  TIM_MasterConfigTypeDef sMasterConfig;
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  //GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(port, &GPIO_InitStruct);

  htim_pwm->Instance = timer;
  htim_pwm->Init.Prescaler = prescaler;
  htim_pwm->Init.CounterMode = TIM_COUNTERMODE_UP;
  htim_pwm->Init.Period = period; // 8M/38k = 210
  htim_pwm->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(htim_pwm);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(htim_pwm, &sMasterConfig);

  configChannel(htim_pwm,pulse,channel);
//  TIM_OC_InitTypeDef sConfigOC;
//  sConfigOC.OCMode = TIM_OCMODE_PWM1;
//  sConfigOC.OCIdleState = TIM_OCIDLESTATE_SET;
//  sConfigOC.Pulse = pulse;  // = 210/2  50% duty cycle
//  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
//  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
//  HAL_TIM_PWM_ConfigChannel(htim_pwm, &sConfigOC, channel);

  HAL_TIM_PWM_MspInit(htim_pwm);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm) {
  /* Peripheral clock enable */
  if(htim_pwm->Instance==TIM2)
		__TIM2_CLK_ENABLE();
  else if (htim_pwm->Instance==TIM1)
		__TIM1_CLK_ENABLE();
  else if (htim_pwm->Instance==TIM3)
  		__TIM3_CLK_ENABLE();
}


void pwm_Init(uint32_t pin, TIM_TypeDef* timer, GPIO_TypeDef* port, uint32_t prescaler, uint32_t period, uint32_t pulse, uint32_t channel,TIM_HandleTypeDef *_htim){
  TIM_Init(prescaler, period, pulse, channel,_htim,pin,timer,port);
  HAL_TIM_Base_Start(_htim);
  HAL_TIM_PWM_Start(_htim,channel);
}




