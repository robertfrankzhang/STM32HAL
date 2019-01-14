/*
 * dockingProcess.c
 *
 *  Created on: Jan 12, 2019
 *      Author: robert
 */
#include "state_machine.h"
#include "stm32f1xx.h"
#include "definitions.h"
#include "dockingProcess.h"
#include "CommunicationProtocol.h"
#include "serial.h"
#include "db.h"

uint8_t receiveBuffer[64];

void dockingProc(){
	initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_7,GPIO_NOPULL);
	HAL_GPIO_WritePin(usbEnum,GPIO_PIN_SET);
	if (state != DEAD){
		//Turn all things off
		HAL_GPIO_WritePin(motor,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(proxLED,GPIO_PIN_RESET);
		initPin(GPIOA,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_FREQ_HIGH,GPIO_PIN_6,GPIO_NOPULL);
		HAL_GPIO_WritePin(pulseLED,GPIO_PIN_RESET);

		//Via communication protocol, transfer data. Protocol returns a boolean "success" or "failure". If success, continue, if fail, handle and return to normal
		enum Result result = talk(Data);
		if (result == Unplugged){
			if (state == IS_DISPENSING){
				state = WAITING_FOR_DISPENSE;
			}
			return;
		}

		//Wipe data from memory
		DB_clear();
		DB_init();

		//Set state to dead
		state = DEAD;
	}

	//Charging
	uint8_t saidHelloForPrescription = 0;
	uint8_t realTimeDateReceived = 0;
	uint8_t pillCountReceived = 0;
	uint8_t lockoutPeriodReceived = 0;
	while (1){
		if (!HAL_GPIO_ReadPin(isPluggedInMonitor)){//If unplugged
			return;
		}
//		if (HAL_GPIO_ReadPin(fullyChargedMonitor)){//If fully charged
//			//Via communication protocol, tell dock is fully charged. Use while loop to repeat this until success, still checking if unplugged in inner loop.
//			enum Result result = talk(Charge_Full);
//			if (result == Unplugged){
//				return;
//			}
//		}
		//If detects a new prescription, handle prescription upload. Once uploaded, break
		uint16_t len;
		if((len=getSerialData(receiveBuffer,64))>0){
			if (checkMessageFail(receiveBuffer,len)){
				if (len==4 && receiveBuffer[3]==Hello){
					saidHelloForPrescription = 1;
				}
				if (receiveBuffer[3]==RealTimeDate){
					realTimeDateReceived = 1;
					//Set Real Time Date
				}
				if (len==5 && receiveBuffer[3]==PillCount){
					pillCountReceived = 1;
					prescriptionData.pillCount = receiveBuffer[4];
				}
				if (len==5 && receiveBuffer[3]==LockoutPeriod){
					lockoutPeriodReceived = 1;
					prescriptionData.lockoutPeriod = receiveBuffer[4];
				}
				//if (receiveBuffer[3]==Done){
					if (realTimeDateReceived && pillCountReceived && lockoutPeriodReceived){
						//HAL_Delay(100);
						sendEmptyMessage(AllReceived);
						break;
					}
					//realTimeDateReceived = 0;
					//pillCountReceived = 0;
					//lockoutPeriodReceived = 0;
				//}
			}
		}

		if(saidHelloForPrescription == 1){
			saidHelloForPrescription = 0;
			sendEmptyMessage(Ready);
		}
	}

	//Set state to whatever the prescription starts with (likely need a prescription parser to translate into alerts)
	//Set as WAITING_FOR_DISPENSE for now
	state = WAITING_FOR_DISPENSE;

	//Turn light on a solid color
	HAL_GPIO_WritePin(pulseLED,GPIO_PIN_SET);

	while (HAL_GPIO_ReadPin(isPluggedInMonitor)){
		//Do Nothing
	}

	//Go into normal functioning mode
	state_machine_init();
	initPin(GPIOA,GPIO_MODE_ANALOG,GPIO_SPEED_FREQ_MEDIUM,GPIO_PIN_7,GPIO_NOPULL);




}
