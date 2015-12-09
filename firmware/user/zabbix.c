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

static char host[] = WIFI_AP_NAME;
static char base64host[200];
static char base64key[200];
static char base64value[200];
