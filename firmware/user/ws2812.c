#include "ws2812.h"
#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_config.h"

#define cli() __asm__("rsil a2, 15")
#define sei() __asm__("rsil a2, 0")
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define RES 6000    // Width of the low gap between bits to cause a frame to latch

void  SEND_WS_0()
{
	uint8_t time;
	time = 3; while(time--) WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	time = 8; while(time--) WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );
}

void  SEND_WS_1()
{
	uint8_t time;
	time = 7; while(time--) WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	time = 5; while(time--) WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );
}

// https://github.com/bigjosh/SimpleNeoPixelDemo/blob/master/SimpleNeopixelDemo/SimpleNeopixelDemo.ino

void sendBit(bool bitVal) {
	if(bitVal == 0) {
		SEND_WS_1();
	} else {
		SEND_WS_0();
	}
}

void sendByte( unsigned char byte ) {

    for( unsigned char bit = 0 ; bit < 8 ; bit++ ) {

      sendBit( bitRead( byte , 7 ) );                // Neopixel wants bit in highest-to-lowest order
                                                     // so send highest bit (bit #7 in an 8-bit byte since they start at 0)
      byte <<= 1;                                    // and then shift left so bit 6 moves into 7, 5 moves into 6, etc

    }
}

void sendPixel( unsigned char r, unsigned char g , unsigned char b )  {

  sendByte(g); // Neopixel wants colors in green then red then blue order
  sendByte(r);
  sendByte(b);

}

void show() {
	ets_delay_us( (RES / 1000UL) + 1);
}

void wsShowColor(unsigned char r, unsigned char g, unsigned char b) {
	cli();
	for(int i = 0; i < PIXELS; i++) {
		sendPixel(r, g, b);
	}
	sei();
	show();
}
