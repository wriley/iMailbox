#ifndef USER_ZABBIX_H_
#define USER_ZABBIX_H_
#include "user_interface.h"
#include "espconn.h"

typedef struct zabbixPayload {
	char *data;
	int data_length;
} zabbixPayload;


void sendToZabbix(void);

#endif /* USER_ZABBIX_H_ */
