/*
 * definitions.h
 *
 *  Created on: Jan 1, 2019
 *      Author: robert
 */

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_
/*
#define button GPIOA,GPIO_PIN_9
#define motor GPIOA,GPIO_PIN_10
#define tiltSwitch GPIOA,GPIO_PIN_8
#define proxLED GPIOA,GPIO_PIN_5
#define pulseLED GPIOA,GPIO_PIN_6
#define fullyChargedMonitor GPIOA,GPIO_PIN_2
#define isPluggedInMonitor GPIOA,GPIO_PIN_15
#define usbEnum GPIOA,GPIO_PIN_7
*/

#define batteryVoltageADC ADC_CHANNEL_0
#define irReceiverADC ADC_CHANNEL_1
#define motorAFLTADC ADC_CHANNEL_2
#define motorBFLTADC ADC_CHANNEL_8

#define isPluggedIn GPIOA,GPIO_PIN_8//FAKE PIN
#define isPluggedInPin GPIO_PIN_8
#define motorSleep GPIOB,GPIO_PIN_1
#define irReceiverPower GPIOB,GPIO_PIN_4
#define lockButton GPIOB,GPIO_PIN_6
#define lockButtonPin GPIO_PIN_6
#define lockButtonPort GPIOB
#define chargeIndicatorInput GPIOB,GPIO_PIN_8 //When charging it is 0. When charge is full it is 1
#define IRLED GPIOB,GPIO_PIN_9
#define chargeLED GPIOB,GPIO_PIN_10
#define dispenseLED GPIOB,GPIO_PIN_11
#define userSwitch GPIOB,GPIO_PIN_13
#define userSwitchPin GPIO_PIN_13
#define userSwitchPort GPIOB

#define lockMotorA1 GPIOA,GPIO_PIN_3
#define lockMotorA2 GPIOA,GPIO_PIN_4
#define dispenseMotorB2 GPIOA,GPIO_PIN_5
#define dispenseMotorB1 GPIOA,GPIO_PIN_6

#define motorFault GPIOA,GPIO_PIN_7

#endif /* DEFINITIONS_H_ */
