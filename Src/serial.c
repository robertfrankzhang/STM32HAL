#include <stdio.h>
#include <stdarg.h>
#include "serial.h"
void serial_init(void){
}

void serial_printf(const char *fmt,...){
  va_list ap;
  va_start(ap,fmt);
  char line[32];
  vsnprintf(line,32,fmt,ap);
  va_end(ap);
  CDC_Transmit_FS(line,strlen(line));
}
