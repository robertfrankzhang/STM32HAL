#include "db.h"
#include "rtc.h"

DB_t _db;

void DB_init(void){
	DB_clear();
}

void DB_clear(void){
	_db.index = 0;
}

int8_t DB_add(DB_Event_t event){
	if(_db.index >= DB_ITEM_MAX)
		return -1; // fail
	RTC_TimeTypeDef time;
	HAL_RTC_GetTime(&hrtc, &time,  RTC_FORMAT_BIN);
	RTC_DateTypeDef date;
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	_db.item[_db.index].date = date;
	_db.item[_db.index].time = time;
	_db.item[_db.index].event = event;
	++_db.index;
	return 0; // ok
}

DB_t *DB_get(void){
	return &_db;
}



