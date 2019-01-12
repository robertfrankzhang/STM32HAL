#ifndef _serial_h
#define _serial_h
#include "usbd_cdc_if.h"
#include <stdint.h>
void serial_init(void);
#define serial_send(data,len) CDC_Transmit_FS(data,len)
void serial_printf(const char *fmt,...);
#endif // _serial_h
