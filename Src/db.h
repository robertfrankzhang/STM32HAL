#ifndef _db_h
#define _db_h
#include <stdint.h>
#include "stm32f1xx.h"

typedef enum {
	Event_NOTALLOWED,
	Event_ALLOWED,
	Event_FORCED
} DB_Event_t;

typedef enum {
	Dispense_SUCCESS,
	Dispense_FAIL
} DB_DispenseStatus_t;

typedef struct {
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
	DB_Event_t event;
	DB_DispenseStatus_t dispenseStatus;
} DB_Item_t;

#define DB_ITEM_MAX 200
typedef struct {
	DB_Item_t item[DB_ITEM_MAX];
	uint16_t index;
} DB_t;

typedef enum {
	Opmode_ASNEEDED,
	Opmode_IMPULSEPROOF,
	Opmode_LOCKED
} OperatingMode_t;

typedef struct{
	int lockoutPeriod; //distance between single dispenses
	uint8_t totalPillCount; //original # of pills
	uint8_t pillCount; //total # of pills left
	uint8_t impulseProofHoldTime; //seconds to hold for impulse proof mode
	OperatingMode_t operatingMode; //operating mode
	uint8_t dockID; //ID of the dock that uploaded the prescription to the device
	uint8_t prescriptionID; //ID of the prescription uploaded
	uint8_t hasPrescriptionUploaded; //Is a prescription uploaded on the device
	uint8_t hasPrescriptionHistory; //Has the device collected any data yet
} PrescriptionData_t;

void DB_init(void);
void DB_clear(void);
int8_t DB_add(DB_Event_t event);

extern DB_t _db;
extern PrescriptionData_t prescriptionData;

#endif // _db_h

