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

uint32_t currentColor = 0;
uint8_t currentWheelPosition = 0;

void  send_ws_0()
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

void  send_ws_1()
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
		send_ws_1();
	} else {
		send_ws_0();
	}
}

void sendByte( uint8_t byte ) {
	//os_printf("%s\n", __FUNCTION__);
    for( uint8_t bit = 0 ; bit < 8 ; bit++ ) {

      sendBit( bitRead( byte , 7 ) );                // Neopixel wants bit in highest-to-lowest order
                                                     // so send highest bit (bit #7 in an 8-bit byte since they start at 0)
      byte <<= 1;                                    // and then shift left so bit 6 moves into 7, 5 moves into 6, etc

    }
}

void sendPixel( uint8_t r, uint8_t g , uint8_t b )  {
	//os_printf("%s\n", __FUNCTION__);
	sendByte(g); // Neopixel wants colors in green then red then blue order
	sendByte(r);
	sendByte(b);

}

void show() {
	ets_delay_us( (RES / 1000UL) + 1);
}

void wsShowColor(uint8_t r, uint8_t g, uint8_t b) {
	//os_printf("%s\n", __FUNCTION__);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(WSGPIO), 0);
	cli();
	for(int i = 0; i < WSPIXELS; i++) {
		sendPixel(r, g, b);
	}
	sei();
	show();
}

uint32_t rgbToColor(uint8_t r, uint8_t g, uint8_t b) {
	return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

void colorToRGB(uint8_t *buf, uint32_t c) {
	buf[0] = (uint8_t)((c >> 16) & 0xff);
	buf[1] = (uint8_t)((c >> 8) & 0xff);
	buf[2] = (uint8_t)(c & 0xff);
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
	currentColor = rgbToColor(r, g, b);
	wsShowColor(r, g, b);
}

unsigned long getColor(void) {
	return currentColor;
}

uint32_t Wheel(uint8_t WheelPos) {
	WheelPos = 255 - WheelPos;
	if(WheelPos < 85) {
		return rgbToColor(255 - WheelPos * 3, 0, WheelPos * 3);
	} else if(WheelPos < 170) {
		WheelPos -= 85;
		return rgbToColor(0, WheelPos * 3, 255 - WheelPos * 3);
	} else {
		WheelPos -= 170;
		return rgbToColor(WheelPos * 3, 255 - WheelPos * 3, 0);
	}
}

void wsRGBFadeNext(void) {
	uint32_t newColor = Wheel(currentWheelPosition++);
	uint8_t rgb[3];
	colorToRGB(rgb, newColor);
	setColor(rgb[0], rgb[1], rgb[2]);
}
