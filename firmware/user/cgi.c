/*
Some random cgi routines. Used in the LED example and the page that returns the entire
flash as a binary. Also handles the hit counter on the main page.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include <ip_addr.h>
#include "espmissingincludes.h"
#include "status.h"

void ICACHE_FLASH_ATTR tplIndex(HttpdConnData *connData, char *token, void **arg)
{
       char buff[128];
       if (token==NULL) return;

       struct iMailboxStatus currentStatus = getStatus();
       long ut = currentStatus.uptimeSeconds;

       if (os_strcmp(token, "uptimeSeconds")==0) {
               os_sprintf(buff, "%ld", ut);
       }
       httpdSend(connData, buff, -1);
}

static char currLedState=1;
static char currLedShow=0;

//Cgi that turns the LED on or off according to the 'led' param in the POST data
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->postBuff, "led", buff, sizeof(buff));
	if (len!=0) {
		currLedState=atoi(buff);
		ioLed(currLedState);
	}

	httpdRedirect(connData, "led.tpl");
	return HTTPD_CGI_DONE;
}


//Template code for the led page.
void ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledstate")==0) {
		if (currLedState) {
			os_strcpy(buff, "off");
		} else {
			os_strcpy(buff, "on");
		}
	}
	httpdSend(connData, buff, -1);
}

//Cgi that reads the SPI flash. Assumes 512KByte flash.
int ICACHE_FLASH_ATTR cgiReadFlash(HttpdConnData *connData) {
	int *pos=(int *)&connData->cgiData;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	if (*pos==0) {
		os_printf("Start flash download.\n");
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "application/bin");
		httpdEndHeaders(connData);
		*pos=0x40200000;
		return HTTPD_CGI_MORE;
	}
	//Send 1K of flash per call. We will get called again if we haven't sent 512K yet.
	espconn_sent(connData->conn, (uint8 *)(*pos), 1024);
	*pos+=1024;
	if (*pos>=0x40200000+(512*1024)) return HTTPD_CGI_DONE; else return HTTPD_CGI_MORE;
}

int ICACHE_FLASH_ATTR cgiStatus(HttpdConnData *connData) {
	int len;
	char buff[1024];
	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);

	struct iMailboxStatus myStatus = getStatus();

	len=os_sprintf(buff, "{\n \"result\": { "
			"\n\"ledMode\": \"%d\","
			"\n\"ledShow\": \"%d\","
			"\n\"brightness\": \"%d\","
			"\n\"lightReading\": \"%d\","
			"\n\"lightThreshold\": \"%d\","
			"\n\"batteryStatus\": \"%d\","
			"\n\"uptimeSeconds\": \"%d\","
			"\n\"freeHeap\": \"%u\""
			"\n }\n}\n",
			myStatus.ledMode,
			myStatus.ledShow,
			myStatus.brightness,
			myStatus.lightReading,
			myStatus.lightThreshold,
			myStatus.batteryStatus,
			myStatus.uptimeSeconds,
			system_get_free_heap_size()
			);
	httpdSend(connData, buff, len);

	return HTTPD_CGI_DONE;
}

void ICACHE_FLASH_ATTR tplSetColor(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	uint32_t currentColor = getColorSingle();

	os_strcpy(buff, "00000");
	if (os_strcmp(token, "currentColor")==0) {
		os_sprintf((char *)&buff, "%06x", currentColor);
	}
	httpdSend(connData, buff, -1);
}

int ICACHE_FLASH_ATTR cgiSetColor(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	char r, g, b;
	os_printf("%s: %s\n", __FUNCTION__, buff);

	len=httpdFindArg(connData->postBuff, "rgb_r", buff, sizeof(buff));
	if (len!=0) {
		r = atoi(buff);
	}

	len=httpdFindArg(connData->postBuff, "rgb_g", buff, sizeof(buff));
	if (len!=0) {
		g = atoi(buff);
	}

	len=httpdFindArg(connData->postBuff, "rgb_b", buff, sizeof(buff));
	if (len!=0) {
		b = atoi(buff);
	}

	os_printf("%s: 0x%02x 0x%02x 0x%02x\n", __FUNCTION__, r, g, b);
	uint32_t rgb = ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
	setColorSingle(rgb);

	httpdRedirect(connData, "/admin/setcolor.tpl");
	return HTTPD_CGI_DONE;
}

void ICACHE_FLASH_ATTR tplAdminIndex(HttpdConnData *connData, char *token, void **arg)
{
       char buff[128];
       if (token==NULL) return;

       struct iMailboxStatus currentStatus = getStatus();
       uint8_t currentMode = currentStatus.ledMode;

       if (os_strcmp(token, "ledMode")==0) {
               os_sprintf(buff, "%d", currentMode);
       }
       httpdSend(connData, buff, -1);
}

int ICACHE_FLASH_ATTR cgiLEDMode(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->postBuff, "ledMode", buff, sizeof(buff));
	if (len!=0) {
		uint8_t m = atoi(buff);
		setMode(m);
		//os_printf("%s: %d\n", __FUNCTION__, m);
	}

	httpdRedirect(connData, "index.tpl");
	return HTTPD_CGI_DONE;
}

void ICACHE_FLASH_ATTR tplSetLightThreshold(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	uint16_t currentLightThreshold = getLightThreshold();

	if (os_strcmp(token, "currentLightThreshold")==0) {
		os_sprintf((char *)&buff, "%d", currentLightThreshold);
	}
	httpdSend(connData, buff, -1);
}

int ICACHE_FLASH_ATTR cgiSetLightThreshold(HttpdConnData *connData) {
	int len;
	char buff[1024];
	uint16_t lightThreshold;

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	os_printf("%s: %s\n", __FUNCTION__, buff);

	len=httpdFindArg(connData->postBuff, "lightThreshold", buff, sizeof(buff));
	if (len!=0) {
		lightThreshold = atoi(buff);
	}

	os_printf("%s: %d\n", __FUNCTION__, lightThreshold);

	setLightThreshold(lightThreshold);

	httpdRedirect(connData, "/admin/setlightthreshold.tpl");
	return HTTPD_CGI_DONE;
}

void ICACHE_FLASH_ATTR tplLedShow(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledShow")==0) {
		os_sprintf(buff, "%d", currLedShow);
	}
	httpdSend(connData, buff, -1);
}

int ICACHE_FLASH_ATTR cgiLedShow(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->postBuff, "ledShow", buff, sizeof(buff));
	if (len!=0) {
		currLedShow=atoi(buff);
		setLedShow(currLedShow);
	}

	httpdRedirect(connData, "setledshow.tpl");
	return HTTPD_CGI_DONE;
}

void ICACHE_FLASH_ATTR tplSetBrightness(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	uint8_t currentBrightness = getBrightness();

	if (os_strcmp(token, "brightness")==0) {
		os_sprintf(buff, "%d", currentBrightness);
	}
	httpdSend(connData, buff, -1);
}

int ICACHE_FLASH_ATTR cgiSetBrightness(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->postBuff, "brightness", buff, sizeof(buff));
	if (len!=0) {
		setBrightness(atoi(buff));
	}

	httpdRedirect(connData, "setbrightness.tpl");
	return HTTPD_CGI_DONE;
}
