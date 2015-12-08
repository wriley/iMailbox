#include <string.h>
#include <osapi.h>
#include "espmissingincludes.h"
#include "httpd.h"
#include "status.h"

// CGI called from index.tpl
int ICACHE_FLASH_ATTR cgiStatus(HttpdConnData *connData) {
	int len;
	int i;
	char buff[1024];
	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);

	iMailboxStatus myStatus = getStatus();

	len=os_sprintf(buff, "{\n \"result\": { "
			"\n\"ledMode\": \"%d\","
			"\n\"ledShow\": \"%d\","
			"\n\"lightReading\": \"%d\","
			"\n\"lightThreshold\": \"%d\","
			"\n\"batteryStatus\": \"%d\","
			"\n\"uptimeSeconds\": \"%d\""
			"\n }\n}\n",
			myStatus.ledMode,
			myStatus.ledShow,
			myStatus.lightReading,
			myStatus.lightThreshold,
			myStatus.batteryStatus,
			myStatus.uptimeSeconds
			);
	httpdSend(connData, buff, len);

	return HTTPD_CGI_DONE;
}
