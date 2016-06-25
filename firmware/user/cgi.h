#ifndef CGI_H
#define CGI_H

#include "httpd.h"

void tplIndex(HttpdConnData *connData, char *token, void **arg);
int cgiLed(HttpdConnData *connData);
void tplLed(HttpdConnData *connData, char *token, void **arg);
void tplSetColor(HttpdConnData *connData, char *token, void **arg);
void tplSetLightThreshold(HttpdConnData *connData, char *token, void **arg);
int cgiReadFlash(HttpdConnData *connData);
int cgiStatus(HttpdConnData *connData);
int cgiSetColor(HttpdConnData *connData);
int cgiSetLightThreshold(HttpdConnData *connData);
void tplAdminIndex(HttpdConnData *connData, char *token, void **arg);
int cgiLEDMode(HttpdConnData *connData);
int cgiLedShow(HttpdConnData *connData);
void tplLedShow(HttpdConnData *connData, char *token, void **arg);
int cgiSetBrightness(HttpdConnData *connData);
void tplSetBrightness(HttpdConnData *connData, char *token, void **arg);

#endif
