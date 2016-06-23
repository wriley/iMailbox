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


#include <esp8266.h>
#include "cgi.h"
#include "io.h"
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

//Cgi that turns the LED on or off according to the 'led' param in the POST data
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->post->buff, "led", buff, sizeof(buff));
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
			"\n\"lightReading\": \"%d\","
			"\n\"lightThreshold\": \"%d\","
			"\n\"batteryStatus\": \"%d\","
			"\n\"uptimeSeconds\": \"%lu\","
			"\n\"freeHeap\": \"%u\""
			"\n }\n}\n",
			myStatus.ledMode,
			myStatus.ledShow,
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
		os_sprintf((char *)&buff, "%06lu", currentColor);
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

	char r = 0, g = 0, b = 0;
	os_printf("%s: %s\n", __FUNCTION__, buff);

	len=httpdFindArg(connData->post->buff, "rgb_r", buff, sizeof(buff));
	if (len!=0) {
		r = atoi(buff);
	}

	len=httpdFindArg(connData->post->buff, "rgb_g", buff, sizeof(buff));
	if (len!=0) {
		g = atoi(buff);
	}

	len=httpdFindArg(connData->post->buff, "rgb_b", buff, sizeof(buff));
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

	len=httpdFindArg(connData->post->buff, "ledMode", buff, sizeof(buff));
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
	uint16_t lightThreshold = 0;

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	os_printf("%s: %s\n", __FUNCTION__, buff);

	len=httpdFindArg(connData->post->buff, "lightThreshold", buff, sizeof(buff));
	if (len!=0) {
		lightThreshold = atoi(buff);
	}

	os_printf("%s: %d\n", __FUNCTION__, lightThreshold);

	setLightThreshold(lightThreshold);

	httpdRedirect(connData, "/admin/setlightthreshold.tpl");
	return HTTPD_CGI_DONE;
}
