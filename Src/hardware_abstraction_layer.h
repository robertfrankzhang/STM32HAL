/*
 * hardware_abstraction_layer.h
 *
 *  Created on: Jul 13, 2019
 *      Author: robert
 */

#ifndef HARDWARE_ABSTRACTION_LAYER_H_
#define HARDWARE_ABSTRACTION_LAYER_H_

uint32_t getADC(uint32_t channel); //returns the ADC value given an ADC channel
void setAlarm(int delay); //given a delay (in seconds), it will set a new RTC alarm. If a previous alarm has not expired, this overwrites it.
void setPinsOfPortToLowPowe(GPIO_TypeDef* port, uint32_t pin1, uint32_t pin2, uint32_t pin3);//pin 1-3 are pins to be excluded (like buttons, which need to be kept pulled up/down). Put 0 if pin N/A
void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull); //initializes a single pin
void disableClocks();

#endif /* HARDWARE_ABSTRACTION_LAYER_H_ */
