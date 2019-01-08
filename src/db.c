#include "db.h"

DB_t _db;

void DB_init(void){
	DB_clear();
}

void DB_clear(void){
	_db.index = 0;
}

int8_t DB_add(DB_Date_t *date, DB_Time_t *time, DB_Event_t event){
	if(_db.index >= DB_ITEM_MAX)
		return -1; // fail
	_db.item[_db.index].date = *date;
	_db.item[_db.index].time = *time;
	_db.item[_db.index].event = event;
	++_db.index;
	return 0; // ok
}

DB_t *DB_get(void){
	return &_db;
}

