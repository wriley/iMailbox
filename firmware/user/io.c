
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "gpio.h"
#include "espmissingincludes.h"

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

#define LEDGPIO 0

void ioInit() {
	// outputs
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

	// inputs w/pull up
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO4_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO5_U);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_MTDI_U);
}

void ICACHE_FLASH_ATTR ioLed(int ena) {
	//gpio_output_set is overkill. ToDo: use better mactos
	if (ena) {
		gpio_output_set((1<<LEDGPIO), 0, (1<<LEDGPIO), 0);
	} else {
		gpio_output_set(0, (1<<LEDGPIO), (1<<LEDGPIO), 0);
	}
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
