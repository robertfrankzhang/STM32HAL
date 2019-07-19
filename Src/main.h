/*
 * main.h
 *
 *  Created on: Jul 13, 2019
 *      Author: robert
 */

#ifndef MAIN_H_
#define MAIN_H_
#include "definitions.h"
#include "state_machine.h"

extern ADC_HandleTypeDef hadc1;

extern RTC_HandleTypeDef hrtc;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;


void initAllPins(void);

#endif /* MAIN_H_ */
