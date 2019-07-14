/*
 * main.h
 *
 *  Created on: Jul 13, 2019
 *      Author: robert
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "state_machine.h"

extern ADC_HandleTypeDef hadc1;
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim2;
void initAllPins(void);

#endif /* MAIN_H_ */
