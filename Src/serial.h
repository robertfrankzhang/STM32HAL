#ifndef _serial_h
#define _serial_h

#include <stdint.h>
void serial_init(void);
uint8_t serial_send(uint8_t *data,uint16_t len);
uint8_t serial_printf(const char *fmt,...);

/* call from usbd_cdc_if.c - CDC_Receive_FS() */
void serial_rcvd(uint8_t* data, uint32_t len);

#define SERIAL_MSGMAX 64

/* api for state_machine */
/* uint8_t rcvddata[64] */
/* uint16_t rcvedlen; */
/* rcvdlen = getSerialData(rcvdbuf,64); */
/* if( rcvdlen > 0) { dosomething }; */
uint16_t getSerialData(uint8_t *buf,uint16_t buflen);
#endif // _serial_h
