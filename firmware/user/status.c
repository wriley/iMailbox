#include "espmissingincludes.h"
#include <osapi.h>
#include "user_interface.h"
#include "status.h"
#include "io.h"
#include "zabbix.h"
#include "spi_flash.h"

#define BATTCHARGEGPIO 4
#define BATTDONEGPIO 5
#define BATTPGGPIO 12

// EEPROM addresses
#define CONFIG_SECTOR 0x3c
#define CONFIG_ADDRESS (CONFIG_SECTOR * SPI_FLASH_SEC_SIZE)
#define EEPROM_OFFSET_MODE 0
#define EEPROM_OFFSET_THRESHOLD 1

static struct iMailboxStatus myStatus;

void incrementUptimeSeconds(void) {
	myStatus.uptimeSeconds++;
	/*
	if((myStatus.uptimeSeconds % 10) == 0) {
		os_printf("freeHeap: %d\n", system_get_free_heap_size());
	}
	*/
}

long getUptimeSeconds(void) {
	return myStatus.uptimeSeconds;
}

void updateLightReading(void) {
	myStatus.lightReading = system_adc_read();
	//os_printf("%s: %d\n", __FUNCTION__, myStatus.lightReading);
}

void updateBatteryStatus(void) {
	 uint8 battChg = !digitalRead(BATTCHARGEGPIO);
	 uint8 battDone = !digitalRead(BATTDONEGPIO);
	 uint8 battPG = !digitalRead(BATTPGGPIO);
	 myStatus.batteryStatus = battChg + (battDone * 2) + (battPG * 4);
}

struct iMailboxStatus getStatus(void) {
	updateBatteryStatus();
	return myStatus;
}

void updateStatus(void) {
	updateLightReading();
	updateBatteryStatus();
}

char getMode(void) {
	return myStatus.ledMode;
}

void setMode(char m) {
	myStatus.ledMode = m;
	saveStatus();
}

uint32_t getColorSingle(void) {
	return myStatus.colorSingle;
}

void setColorSingle(uint32_t c) {
	myStatus.colorSingle = c;
	myStatus.ledMode = SINGLECOLOR;
	saveStatus();
}

uint16_t getLightThreshold(void) {
	return myStatus.lightThreshold;
}

void setLightThreshold(uint16_t t) {
	myStatus.lightThreshold = t;
	saveStatus();
}

uint16_t getLightReading(void) {
	return myStatus.lightReading;
}

void setLedShow(uint8_t s) {
	myStatus.ledShow = s;
}

uint8_t getLedShow(void) {
	return myStatus.ledShow;
}

void loadStatus(void) {
	uint32_t oldUptime = myStatus.uptimeSeconds;

	SpiFlashOpResult result = spi_flash_read(CONFIG_ADDRESS, (uint32 *)&myStatus, sizeof(struct iMailboxStatus));
	os_printf("%s: Read result - %d\n", __FUNCTION__, result);

	if(result == SPI_FLASH_RESULT_OK) {
		os_printf("%s: ledMode: %d\n", __FUNCTION__, myStatus.ledMode);
		os_printf("%s: ledShow: %d\n", __FUNCTION__, myStatus.ledShow);
		os_printf("%s: lightReading: %d\n", __FUNCTION__, myStatus.lightReading);
		os_printf("%s: lightThreshold: %d\n", __FUNCTION__, myStatus.lightThreshold);
		os_printf("%s: batteryStatus: %d\n", __FUNCTION__, myStatus.batteryStatus);
		os_printf("%s: uptimeSeconds: %ld\n", __FUNCTION__, myStatus.uptimeSeconds);
		os_printf("%s: colorSingle: %ld\n", __FUNCTION__, myStatus.colorSingle);
		os_printf("%s: colorFade1: %ld\n", __FUNCTION__, myStatus.colorFade1);
		os_printf("%s: colorFade2: %ld\n", __FUNCTION__, myStatus.colorFade2);

		setMode(myStatus.ledMode);
		myStatus.uptimeSeconds = oldUptime;
		if(myStatus.lightThreshold == 65535) {
			os_printf("%s: lightThreshold not set so using default\n", __FUNCTION__);
			myStatus.lightThreshold = 600;
		}
		if(myStatus.ledMode < 0 || myStatus.ledMode > 4) {
			os_printf("%s: ledMode not set so using default\n", __FUNCTION__);
			myStatus.ledMode = 0;
		}
		if(myStatus.ledShow < 0 || myStatus.ledShow > 1) {
			os_printf("%s: ledShow not set so using default\n", __FUNCTION__);
			myStatus.ledShow = 0;
		}
	}
}

void saveStatus(void) {
	SpiFlashOpResult result = spi_flash_erase_sector(CONFIG_SECTOR);
	os_printf("%s: Erase result - %d\n", __FUNCTION__, result);
	if(result == SPI_FLASH_RESULT_OK) {
		result = spi_flash_write(CONFIG_ADDRESS, (uint32 *)&myStatus, sizeof(myStatus));
		os_printf("%s: Write result - %d\n", __FUNCTION__, result);
	}
}
