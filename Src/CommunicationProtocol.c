/*
 * CommunicationProtocol.c
 *
 *  Created on: Jan 12, 2019
 *      Author: robert
 */
#include "CommunicationProtocol.h"
#include <stdint.h>
#include "dockingProcess.h"
#include "definitions.h"
#include "stm32f1xx.h"
#include "db.h"
#include "serial.h"
#include <string.h>

uint8_t checkMessageFail(uint8_t* checkBuffer,uint8_t bufferLength){
	return (checkBuffer[0] == 0xAA && checkBuffer[1] == 0xBB && bufferLength >= 4);
}

void sendMessage(uint8_t* messageBody,uint8_t messageBodyLength, enum Command command){
	uint8_t fullMsg[64];
	fullMsg[0] = 0xAA;
	fullMsg[1] = 0xBB;
	fullMsg[2] = messageBodyLength+1;
	fullMsg[3] = command;
	if (messageBodyLength > 0){
		memcpy(&fullMsg[4],messageBody,messageBodyLength);
	}
	serial_send(fullMsg,messageBodyLength+4);
}

void sendEmptyMessage(enum Command command){
	sendMessage(0,0,command);
}

enum Result talk(enum Action command){//Loop process of Hello, Receive Affirm, Send Info, Receive All_Good until success or unplugged
	while (1){
		sendEmptyMessage(Hello);
		uint8_t len;
		uint8_t counter = 0;
		while ((len=getSerialData(receiveBuffer,64))==0){
			HAL_Delay(10);
//			if (!HAL_GPIO_ReadPin(isPluggedInMonitor)){//If unplugged
//				return Unplugged;
//			}
			if (counter > 100){
				break;
			}
			++counter;
		}
		//if received a "Ready" then continue else send Hello again
		if (len==4 && receiveBuffer[3]==Ready && checkMessageFail(receiveBuffer,len)){
			break;
		}
	}
	while (1){
		if (command == Data){
			//Send timestamps line by line
			for (int i = 0; i<_db.index; i++){
				uint8_t messageBody[8] = {_db.item[i].event,_db.item[i].date.WeekDay,
										  _db.item[i].date.Date,_db.item[i].date.Month,
										  _db.item[i].date.Year,_db.item[i].time.Hours,
										  _db.item[i].time.Minutes,_db.item[i].time.Seconds};
				sendMessage(messageBody,8,Timestamp);
				HAL_Delay(100);
			}
		}else if (command == Charge_Full){
			sendEmptyMessage(Full);
		}
		sendEmptyMessage(Done);

		uint8_t len;
		uint8_t counter = 0;
		while ((len=getSerialData(receiveBuffer,64))==0){
			HAL_Delay(10);
//			if (!HAL_GPIO_ReadPin(isPluggedInMonitor)){//If unplugged
//				return Unplugged;
//			}
			if (counter > 100){
				break;
			}
			++counter;
		}

		//if received an "All Good" then return Success else send info again
		if (len==4 && receiveBuffer[3]==AllReceived && checkMessageFail(receiveBuffer,len)){
			return Success;
		}
	}

	return Success;
}

/*List of Commands
 * Hello: Tell receiver you want to send message
 * Ready: Tell receiver you received their hello and are ready to receive message body
 * Done: Tell receiver you have finished sending all of your message body
 * AllReceived: Tell receiver you have successfully received their message body
 * Full: Tell receiver the device is finished charging
 * TimeStamp: Preface a timestamp
 * RealTimeDate: Preface a real time date and time to calibrate RTC
 * PillCount: Number of Pills in Prescription
 * LockoutPeriod: Seconds to lockout prescription
 *
 *
 * *
 */
