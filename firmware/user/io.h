#ifndef IO_H
#define IO_H

void ICACHE_FLASH_ATTR ioLed(int ena);
void ioInit(void);

// added
uint8 digitalRead(uint8);
void digitalWrite(uint8, uint8);

#endif
