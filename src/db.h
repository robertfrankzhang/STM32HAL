#ifndef _db_h
#define _db_h
#include <stdint.h>

typedef enum {
	Event_Push,
	Event_Dispensed
} DB_Event_t;

typedef struct {
	uint8_t WeekDay;
	uint8_t Month;
	uint8_t Date;
	uint8_t Year;
} DB_Date_t;

typedef struct {
	uint8_t Hours;
	uint8_t Minutes;
	uint8_t Seconds;
} DB_Time_t;

typedef struct {
	DB_Date_t date;
	DB_Time_t time;
	DB_Event_t event;
} DB_Item_t;

#define DB_ITEM_MAX 1000
typedef struct {
	DB_Item_t item[DB_ITEM_MAX];
	uint16_t index;
} DB_t;

void DB_init(void);
void DB_clear(void);
int8_t DB_add(DB_Date_t *date, DB_Time_t *time, DB_Event_t event);
DB_t *DB_get(void);

#endif // _db_h
