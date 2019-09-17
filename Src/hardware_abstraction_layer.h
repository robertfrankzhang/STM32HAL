/*
 * hardware_abstraction_layer.h
 *
 *  Created on: Jul 13, 2019
 *      Author: robert
 */

#ifndef HARDWARE_ABSTRACTION_LAYER_H_
#define HARDWARE_ABSTRACTION_LAYER_H_
#include "definitions.h"
#include "stm32f1xx.h"
#include "main.h"


uint32_t getADC(uint32_t channel); //returns the ADC value given an ADC channel
void setAlarm(int delay); //given a delay (in seconds), it will set a new RTC alarm. If a previous alarm has not expired, this overwrites it.
void setPinsOfPortToLowPowe(GPIO_TypeDef* port, uint32_t pin1, uint32_t pin2, uint32_t pin3);//pin 1-3 are pins to be excluded (like buttons, which need to be kept pulled up/down). Put 0 if pin N/A
void initPin(GPIO_TypeDef* port, uint32_t mode, uint32_t speed, uint32_t pin, uint32_t pull); //initializes a single pin
void disableClocks();

// motor oprations

#ifdef USE_MOTOR_PWM
// 50KHz=48MHz/48/20
// define PWM prescale and period must match definition in MX
#define MOTOR_PWM_PRESCALE 48
#define MOTOR_PWM_PERIOD   20

#define MOTOR_PWM_PULSE_UP 5
#define MOTOR_PWM_PULSE_DN 15

//#define MOTOR_PWM_PULSE_DN (MOTOR_PWM_PERIOD-MOTOR_PWM_PULSE_UP)
#endif

void spinDispenseMotor(int8_t isForward);
void stopDispenseMotor(void);

#define dispenseMotorCurrent() getADC(motorBFLTADC)
uint8_t dispenseMotorCurrentAverage(uint8_t len);

uint8_t motorIsFault();
#define batteryIsCharging() (HAL_GPIO_ReadPin(chargeIndicatorInput) == 0)
#define batteryLevel() (getADC(batteryVoltageADC)*330/(4096/2))

#define setPwmPulse(htim_pwm,channel,pulse) __HAL_TIM_SET_COMPARE(htim_pwm,channel,pulse)
void startIRSamplingTimer();
void stopIRSamplingTimer();

#define usbHostConnect() HAL_GPIO_WritePin(usbEnum,GPIO_PIN_SET)
#define usbHostDisconnect() HAL_GPIO_WritePin(usbEnum,GPIO_PIN_RESET)
#define usbIsPluggedIn() (HAL_GPIO_ReadPin(isPluggedIn) != 0)

#endif /* HARDWARE_ABSTRACTION_LAYER_H_ */
