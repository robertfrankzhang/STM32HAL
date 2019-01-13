/*
 * CommunicationProtocol.h
 *
 *  Created on: Jan 12, 2019
 *      Author: robert
 */
#include <stdint.h>

#ifndef COMMUNICATIONPROTOCOL_H_
#define COMMUNICATIONPROTOCOL_H_
enum Action{
	Data,Charge_Full
};

enum Result{
	Success,Unplugged
};

enum Command{
	Hello = 1,Ready = 2, Done = 3, AllReceived = 4, Full = 5, Timestamp = 6, RealTimeDate = 7, PillCount = 8, LockoutPeriod = 9
};

uint8_t checkMessageFail(uint8_t* checkBuffer,uint8_t bufferLength);
void sendMessage(uint8_t* messageBody,uint8_t messageBodyLength, enum Command command);
enum Result talk(enum Action command);
void sendEmptyMessage(enum Command command);

#endif /* COMMUNICATIONPROTOCOL_H_ */
