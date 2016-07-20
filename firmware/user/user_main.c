

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "httpd.h"
#include "io.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "stdout.h"
#include "auth.h"
#include "wifi.h"
#include "status.h"
#include "ws2812.h"
#include "zabbix.h"


//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		os_strcpy(user, "admin");
		os_strcpy(pass, "s3cr3t");
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		os_strcpy(user, "user1");
//		os_strcpy(pass, "something");
//		return 1;
	}
	return 0;
}


/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.tpl"},
	{"/index.tpl", cgiEspFsTemplate, tplIndex},
	{"/status.cgi", cgiStatus, NULL},
	{"/status", cgiStatus, NULL},

	//Routines to make the /wifi URL and everything beneath it work.

//Enable the line below to protect the WiFi configuration with an username/password combo.
	//{"/wifi/*", authBasic, myPassFn},
	//{"/admin/*", authBasic, myPassFn},

	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifi/setmode.cgi", cgiWifiSetMode, NULL},

	{"/admin", cgiRedirect, "/admin/index.tpl"},
	{"/admin/", cgiRedirect, "/admin/index.tpl"},
	{"/admin/index.tpl", cgiEspFsTemplate, tplAdminIndex},
	{"/admin/ledmode.cgi", cgiLEDMode, NULL},
	{"/admin/setcolor.tpl", cgiEspFsTemplate, tplSetColor},
	{"/admin/setcolor.cgi", cgiSetColor, NULL},
	{"/admin/led.tpl", cgiEspFsTemplate, tplLed},
	{"/admin/led.cgi", cgiLed, NULL},
	{"/admin/setledshow.tpl", cgiEspFsTemplate, tplLedShow},
	{"/admin/setledshow.cgi", cgiLedShow, NULL},
	{"/admin/setlightthreshold.tpl", cgiEspFsTemplate, tplSetLightThreshold},
	{"/admin/setlightthreshold.cgi", cgiSetLightThreshold, NULL},
	{"/admin/setbrightness.tpl", cgiEspFsTemplate, tplSetBrightness},
	{"/admin/setbrightness.cgi", cgiSetBrightness, NULL},
	{"/admin/flash.bin", cgiReadFlash, NULL},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

// timers
static os_timer_t timerUptime;
static os_timer_t timerZabbix;
static os_timer_t timerLEDMode;
static os_timer_t timerUpdateStatus;

static uint8_t batteryStatusCount = 0;
static uint8_t batteryStatusCountFlag = 0;

void timerFunctionUptime(void *arg) {
	incrementUptimeSeconds();
}

void timerFunctionZabbix(void *arg) {
	struct iMailboxStatus currentStatus = getStatus();

	if((currentStatus.batteryStatus != LOWBATTERY) && (currentStatus.batteryStatus != FAULT)) {
		sendToZabbix();
	}
}

void timerFunctionLEDMode(void *arg) {
	struct iMailboxStatus currentStatus = getStatus();

	if(
		((currentStatus.lightReading < currentStatus.lightThreshold) || (currentStatus.ledShow == 1)) &&
		(currentStatus.batteryStatus != LOWBATTERY) && (currentStatus.batteryStatus != FAULT)
	) {
		char mode = getMode();

		switch(mode) {
			case SINGLECOLOR:
				showColorSingle(getColorSingle());
				break;
			case RGBFADE:
				showColorWheel();
				break;
			case COLORFADE1:
				// TODO
				showColorSingle(0);
				break;
			case COLORFADE2:
				// TODO
				showColorSingle(0);
				break;
			default:
				// Turn off
				showColorSingle(0);
				break;
		}
	} else {
		// Turn off
		showColorSingle(0);
	}
}

void timerFunctionUpdateStatus(void *arg) {
	updateStatus();

	os_printf("%s: batteryStatusCount: %d BatteryStatusCountFlag: %d\n", __FUNCTION__, batteryStatusCount, batteryStatusCountFlag);

	struct iMailboxStatus currentStatus = getStatus();

	if((currentStatus.batteryStatus == LOWBATTERY) || (currentStatus.batteryStatus == FAULT)) {
		batteryStatusCountFlag = 1;
		if(batteryStatusCount < 9) {
			batteryStatusCount++;
		}
	} else {
		if(batteryStatusCount > 0) {
			batteryStatusCount--;
		}
	}

	if(batteryStatusCount == 1) {
		os_printf("Battery low or faulted, disabling LED and Zabbix timers\n");
		// Turn off
		showColorSingle(0);
		// Disable timers
		os_timer_disarm(&timerLEDMode);
		os_timer_disarm(&timerZabbix);
	}

	if((batteryStatusCount == 0) && (batteryStatusCountFlag == 1)) {
		batteryStatusCountFlag = 0;
		os_printf("Enabling LED and Zabbix timers\n");
		os_timer_disarm(&timerLEDMode);
		os_timer_setfn(&timerLEDMode, (os_timer_func_t *)timerFunctionLEDMode, NULL);
		os_timer_arm(&timerLEDMode, 100, 1);

		os_timer_disarm(&timerZabbix);
		os_timer_setfn(&timerZabbix, (os_timer_func_t *)timerFunctionZabbix, NULL);
		os_timer_arm(&timerZabbix, 60000, 1);
	}
}

void timerInit(void) {
	// increment uptime
	os_timer_disarm(&timerUptime);
	os_timer_setfn(&timerUptime, (os_timer_func_t *)timerFunctionUptime, NULL);
	os_timer_arm(&timerUptime, 1000, 1);

	// update LEDs based on mode
	os_timer_disarm(&timerLEDMode);
	os_timer_setfn(&timerLEDMode, (os_timer_func_t *)timerFunctionLEDMode, NULL);
	os_timer_arm(&timerLEDMode, 100, 1);

	// send Zabbix data
	os_timer_disarm(&timerZabbix);
	os_timer_setfn(&timerZabbix, (os_timer_func_t *)timerFunctionZabbix, NULL);
	os_timer_arm(&timerZabbix, 60000, 1);

	// update status
	os_timer_disarm(&timerUpdateStatus);
	os_timer_setfn(&timerUpdateStatus, (os_timer_func_t *)timerFunctionUpdateStatus, NULL);
	os_timer_arm(&timerUpdateStatus, 60000, 1);
}

//Main routine. Initialize stdout, the I/O and the webserver and we're done.
void user_init(void) {
	stdoutInit();
	ioInit();
	loadStatus();
	updateStatus();
	wifiInit();
	httpdInit(builtInUrls, 80);
	timerInit();
	os_printf("\nReady\nfreeHeap: %d\n", system_get_free_heap_size());
}
