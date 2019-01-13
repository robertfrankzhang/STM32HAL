#ifndef _db_h
#define _db_h
#include <stdint.h>
#include "stm32f1xx.h"

typedef enum {
	Event_IDLE,
	Event_ALLOWED
} DB_Event_t;

typedef struct {
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
	DB_Event_t event;
} DB_Item_t;

#define DB_ITEM_MAX 200
typedef struct {
	DB_Item_t item[DB_ITEM_MAX];
	uint16_t index;
} DB_t;

typedef struct{
	int lockoutPeriod;
	uint8_t pillCount;
} PrescriptionData_t;

void DB_init(void);
void DB_clear(void);
int8_t DB_add(DB_Event_t event);

extern DB_t _db;
extern PrescriptionData_t prescriptionData;

#endif // _db_h

