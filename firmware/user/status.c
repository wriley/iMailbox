#include "espmissingincludes.h"
#include "user_interface.h"
#include "status.h"
#include "io.h"
#include "zabbix.h"

#define BATTCHARGEGPIO 4
#define BATTDONEGPIO 5
#define BATTPGGPIO 12

static iMailboxStatus myStatus;

void incrementUptimeSeconds(void) {
	myStatus.uptimeSeconds++;
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

iMailboxStatus getStatus(void) {
	updateBatteryStatus();
	return myStatus;
}

void updateStatus(void) {
	updateLightReading();
	updateBatteryStatus();
}
