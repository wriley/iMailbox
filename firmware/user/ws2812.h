#ifndef _WS2812_H
#define _WS2812_H

#include "ets_sys.h"

// missing
void ets_delay_us(int ms);

void showColorSingle(uint32_t);
void showColorWheel(void);

#endif
