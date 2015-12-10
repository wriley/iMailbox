#include "ws2812.h"
#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_config.h"
#include "gpio.h"

#define cli() __asm__("rsil a2, 3")
#define sei() __asm__("rsil a2, 0")
#define nop() __asm__("nop")

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define RES 6000    // Width of the low gap between bits to cause a frame to latch
#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
	gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)


void  SEND_WS_0()
{
	//os_printf("%s\n", __FUNCTION__);
	uint8_t time;
	time = 3;
	while(time--) {
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<WSGPIO);
	}
	time = 8;
	while(time--) {
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<WSGPIO);
	}
}

void  SEND_WS_1()
{
	//os_printf("%s\n", __FUNCTION__);
	uint8_t time;
	time = 7;
	while(time--) {
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<WSGPIO);
	}
	time = 5;
	while(time--) {
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<WSGPIO);
	}
}

// Parts based on https://github.com/bigjosh/SimpleNeoPixelDemo/blob/master/SimpleNeopixelDemo/SimpleNeopixelDemo.ino

void sendBit(bool bitVal) {
	//os_printf("%s\n", __FUNCTION__);
	if(bitVal == 1) {
		SEND_WS_1();
	} else {
		SEND_WS_0();
	}
}

void sendByte( unsigned char byte ) {
	//os_printf("%s\n", __FUNCTION__);
    for( unsigned char bit = 0 ; bit < 8 ; bit++ ) {

      sendBit( bitRead( byte , 7 ) );                // Neopixel wants bit in highest-to-lowest order
                                                     // so send highest bit (bit #7 in an 8-bit byte since they start at 0)
      byte <<= 1;                                    // and then shift left so bit 6 moves into 7, 5 moves into 6, etc

    }
}

void sendPixel( unsigned char r, unsigned char g , unsigned char b )  {
	//os_printf("%s\n", __FUNCTION__);
	sendByte(g); // Neopixel wants colors in green then red then blue order
	sendByte(r);
	sendByte(b);

}

void show() {
	ets_delay_us( (RES / 1000UL) + 1);
}

void wsShowColor(unsigned char r, unsigned char g, unsigned char b) {
	os_printf("%s\n", __FUNCTION__);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(WSGPIO), 0);
	cli();
	for(int i = 0; i < PIXELS; i++) {
		sendPixel(r, g, b);
	}
	sei();
	show();
}

