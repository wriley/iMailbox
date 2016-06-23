
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>

// from esp8266 Arduino source
/*-----------------------------------------*/
#define PERIPHS_GPIO_BASEADDR	0x60000300

#define GPIO_OUT			(PERIPHS_GPIO_BASEADDR + 0x00)
#define GPIO_OUT_W1TS		(PERIPHS_GPIO_BASEADDR + 0x04)
#define GPIO_OUT_W1TC		(PERIPHS_GPIO_BASEADDR + 0x08)

#define GPIO_ENABLE			(PERIPHS_GPIO_BASEADDR + 0x0C)
#define GPIO_ENABLE_W1TS	(PERIPHS_GPIO_BASEADDR + 0x10)
#define GPIO_ENABLE_W1TC	(PERIPHS_GPIO_BASEADDR + 0x14)

#define GPIO_IN				(PERIPHS_GPIO_BASEADDR + 0x18)
/*-----------------------------------------*/

#define LEDGPIO 2
#define BTNGPIO 0

static ETSTimer resetBtntimer;

void ICACHE_FLASH_ATTR ioLed(int ena) {
	//gpio_output_set is overkill. ToDo: use better mactos
	if (ena) {
		gpio_output_set((1<<LEDGPIO), 0, (1<<LEDGPIO), 0);
	} else {
		gpio_output_set(0, (1<<LEDGPIO), (1<<LEDGPIO), 0);
	}
}

static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg) {
	static int resetCnt=0;
	if (!GPIO_INPUT_GET(BTNGPIO)) {
		resetCnt++;
	} else {
		if (resetCnt>=6) { //3 sec pressed
			wifi_station_disconnect();
			wifi_set_opmode(0x3); //reset to AP+STA mode
			os_printf("Reset to AP mode. Restarting system...\n");
			system_restart();
		}
		resetCnt=0;
	}
}

void ioInit() {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	gpio_output_set(0, 0, (1<<LEDGPIO), (1<<BTNGPIO));
	os_timer_disarm(&resetBtntimer);
	os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
	os_timer_arm(&resetBtntimer, 500, 1);
}

// based on esp8266 Arduino source
uint8 digitalRead(uint8 pin) {
	if (READ_PERI_REG(GPIO_ENABLE) & ((uint32) 0x1 << pin))
		{
			return !!((READ_PERI_REG(GPIO_OUT) & ((uint32) 0x1 << pin)));
		}
		else
		{
			return !!((READ_PERI_REG(GPIO_IN) & ((uint32) 0x1 << pin)));
		}
}

// based on esp8266 Arduino source
void digitalWrite(uint8 pin, uint8 value)
{
		WRITE_PERI_REG(GPIO_OUT, (READ_PERI_REG(GPIO_OUT) & (uint32) (~(0x01 << pin))) | (value << pin));
}
