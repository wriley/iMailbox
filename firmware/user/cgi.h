#ifndef CGI_H
#define CGI_H

#include "httpd.h"

void tplIndex(HttpdConnData *connData, char *token, void **arg);
int cgiLed(HttpdConnData *connData);
void tplLed(HttpdConnData *connData, char *token, void **arg);
void tplSetColor(HttpdConnData *connData, char *token, void **arg);
int cgiReadFlash(HttpdConnData *connData);
int cgiStatus(HttpdConnData *connData);
int cgiSetColor(HttpdConnData *connData);

#endif
