#include "user_interface.h"
#include "status.h"


static iMailboxStatus myStatus;

iMailboxStatus getStatus(void) {
	return myStatus;
}

void incrementUptimeSeconds(void) {
	myStatus.uptimeSeconds++;
}

long getUptimeSeconds(void) {
	return myStatus.uptimeSeconds;
}

void updateLightReading(void) {
	myStatus.lightReading = system_adc_read();
}

void updateBatteryStatus(void) {

}
