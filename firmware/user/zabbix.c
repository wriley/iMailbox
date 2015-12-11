#include "espmissingincludes.h"
#include "zabbix.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "gpio.h"
#include "user_config.h"
#include "stdout.h"
#include "user_config.h"
#include "base64.h"
#include "status.h"

static char base64host[32];
static char base64key[32];
static char base64value[32];


void ICACHE_FLASH_ATTR
at_tcpclient_recon_cb(void *arg, sint8 errType) {
	#ifdef PLATFORM_DEBUG
    os_printf("Reconnect callback\r\n");
    os_printf("Free mem: %d, err %d\r\n", system_get_free_heap_size(), errType);
	#endif
    struct espconn *pespconn = (struct espconn *) arg;
    zabbixPayload *payload = (zabbixPayload *)pespconn->reverse;
    os_free(payload);
    espconn_delete(pespconn);
}

void ICACHE_FLASH_ATTR
at_tcpclient_sent_cb(void *arg) {
	#ifdef PLATFORM_DEBUG
	os_printf("Send callback\r\n");
	#endif
	struct espconn *pespconn = (struct espconn *)arg;
	espconn_disconnect(pespconn);
}

void ICACHE_FLASH_ATTR
at_tcpclient_discon_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *)arg;
	os_free(pespconn->proto.tcp);
	os_free(pespconn);
	#ifdef PLATFORM_DEBUG
	os_printf("Disconnect callback\r\n");
	#endif
}

void ICACHE_FLASH_ATTR
at_tcpclient_connect_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;
	zabbixPayload *payload = (zabbixPayload *)pespconn->reverse;
	#ifdef PLATFORM_DEBUG
	os_printf("TCP client connect\r\n");
	#endif
	espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);
	espconn_regist_disconcb(pespconn, at_tcpclient_discon_cb);

	espconn_sent(pespconn, payload->data, payload->data_length);
}

void ICACHE_FLASH_ATTR
senddata(zabbixPayload *payload)
{
	char info[150];
	char tcpserverip[15];
	struct espconn *pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
	if (pCon == NULL)
	{
		#ifdef PLATFORM_DEBUG
		os_printf("TCP connect failed\r\n");
		#endif
		return;
	}
	pCon->type = ESPCONN_TCP;
	pCon->state = ESPCONN_NONE;
	os_sprintf(tcpserverip, "%s", TCPSERVERIP);
	uint32_t ip = ipaddr_addr(tcpserverip);
	pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	pCon->proto.tcp->local_port = espconn_port();
	pCon->proto.tcp->remote_port = TCPSERVERPORT;
	os_memcpy(pCon->proto.tcp->remote_ip, &ip, 4);
	pCon->reverse = (void*)payload;
	espconn_regist_connectcb(pCon, at_tcpclient_connect_cb);
	espconn_regist_reconcb(pCon, at_tcpclient_recon_cb);
	#ifdef PLATFORM_DEBUG
	os_sprintf(info,"Start espconn_connect to " IPSTR ":%d\r\n",
		   IP2STR(pCon->proto.tcp->remote_ip),
		   pCon->proto.tcp->remote_port);
	os_printf(info);
	#endif
	espconn_connect(pCon);
}

void sendKeyVal(char *key, char *val) {
	os_printf("%s(%s, %s)\n", __FUNCTION__, key, val);
	base64_encode(strlen(key), key, sizeof(base64key), base64key);
	base64_encode(strlen(val), val, sizeof(base64value), base64value);
	struct zabbixPayload *payload = (struct zabbixPayload *)os_zalloc(sizeof(struct zabbixPayload));
	char buffer[128];
	int len = os_sprintf((char *)&buffer,
			"<req>\n <host>%s</host>\n <key>%s</key>\n <data>%s</data>\n</req>\n",
			base64host,
			base64key,
			base64value
			);
	payload->data = (char *)os_zalloc(len*sizeof(char));
	payload->data_length = len;
	os_memcpy(payload->data, &buffer, len);

	senddata(payload);
}

void sendToZabbix() {
	iMailboxStatus currentStatus = getStatus();
	base64_encode(sizeof(WIFI_AP_NAME)-1, WIFI_AP_NAME, sizeof(base64host), base64host);
	char tmp[16];

	// uptime
	os_sprintf((char *)&tmp, "%u", currentStatus.uptimeSeconds);
	sendKeyVal("uptime", (char *)&tmp);

	// free heap
	os_sprintf((char *)&tmp, "%u", system_get_free_heap_size());
	sendKeyVal("freeHeap", (char *)&tmp);

	// lightReading
	os_sprintf((char *)&tmp, "%u", currentStatus.lightReading);
	sendKeyVal("lightReading", (char *)&tmp);

	// batteryStatus
	os_sprintf((char *)&tmp, "%u", currentStatus.batteryStatus);
	sendKeyVal("batteryStatus", (char *)&tmp);
}
