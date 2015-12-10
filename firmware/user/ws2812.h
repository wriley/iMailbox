#ifndef _WS2812_H
#define _WS2812_H

#define WSGPIO 2

void wsShowColor(unsigned char r, unsigned char g, unsigned char b);
int  gpio_ws2812( char *buffer, int length );

#endif
