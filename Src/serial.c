#include <stdio.h>
#include <stdarg.h>
#include "serial.h"
#include "usbd_cdc_if.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

uint8_t serial_rxbuf[SERIAL_MSGMAX];
uint8_t serial_rxlen=0;

void serial_init(void){
  serial_rxlen= 0;
}

uint8_t serial_send(uint8_t *data,uint16_t len){
  if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
    return CDC_Transmit_FS(data,len);
  return 0;
}

uint8_t serial_printf(const char *fmt,...){
  va_list ap;
  va_start(ap,fmt);
  char line[64];
  vsnprintf(line,32,fmt,ap);
  va_end(ap);
  if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED)
    return CDC_Transmit_FS((uint8_t*)line,strlen(line));
  return 0;
}

void serial_rcvd(uint8_t* data, uint32_t len){
	if(len>SERIAL_MSGMAX)
		len = SERIAL_MSGMAX;
	memcpy(serial_rxbuf,data,len);
	serial_rxlen = len;
}

uint16_t getSerialData(uint8_t *buf,uint16_t buflen){
	uint16_t len = serial_rxlen;
	if(len > buflen)
		len = buflen;
	memcpy(buf,serial_rxbuf,len);
	serial_rxlen = 0;
	return len;
}
