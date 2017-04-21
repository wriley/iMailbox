#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <Ticker.h>
#include "config.h"

SoftwareSerial HC12(14, 12);

char HC12ByteIn;
String HC12ReadBuffer = "";
boolean HC12End = false;
uint32_t uptimeSeconds;
byte ledState = HIGH;

WiFiClient espClient;
Ticker uptimeTicker;

struct __attribute__((aligned(4))) iMailboxStatus {
	uint32_t uptimeSeconds;
	uint32_t colorSingle;
	uint32_t colorFade1;
	uint32_t colorFade2;
	uint16_t lightReading;
	uint16_t lightThreshold;
	uint8_t dummy;
	uint8_t ledMode;
	uint8_t ledShow;
	uint8_t batteryStatus;
	uint8_t brightness;
};

iMailboxStatus remoteStatus;

typedef enum {
	ONBATTERY,
	LOWBATTERY,
	ONSOLAR,
	CHARGING,
	CHARGECOMPLETE,
	FAULT
} battery_status_t;

typedef enum {
	OFF,
	SINGLECOLOR,
	RGBFADE,
	COLORFADE1,
	COLORFADE2
} led_mode_t;

void toggleLED() {
	if(ledState == LOW)
		ledState = HIGH;
	else
		ledState = LOW;
  digitalWrite(LED_BUILTIN, ledState);
}

void IncrementUptime() {
  uptimeSeconds++;
	toggleLED();
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch(event) {
        case WIFI_EVENT_STAMODE_GOT_IP:
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case WIFI_EVENT_STAMODE_DISCONNECTED:
            Serial.println("WiFi lost connection");
            break;
    }
}

void DumpStatus() {
	Serial.println("STATUS DUMP:");

	Serial.print("SD* uptimeSeconds: ");
	Serial.println(remoteStatus.uptimeSeconds);

	Serial.print("SD* colorSingle: ");
	Serial.println(remoteStatus.colorSingle);

	Serial.print("SD* colorFade1: ");
	Serial.println(remoteStatus.colorFade1);

	Serial.print("SD* colorFade2: ");
	Serial.println(remoteStatus.colorFade2);

	Serial.print("SD* lightReading: ");
	Serial.println(remoteStatus.lightReading);

	Serial.print("SD* lightThreshold: ");
	Serial.println(remoteStatus.lightThreshold);

	Serial.print("SD* ledMode: ");
	Serial.println(remoteStatus.ledMode);

	Serial.print("SD* ledShow: ");
	Serial.println(remoteStatus.ledShow);

	Serial.print("SD* batteryStatus: ");
	Serial.println(remoteStatus.batteryStatus);

	Serial.print("SD* brightness: ");
	Serial.println(remoteStatus.brightness);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("");
  Serial.println("Beginning setup");

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, ledState);

	uptimeTicker.setCallback(IncrementUptime);
  uptimeTicker.setInterval(1000);
  uptimeTicker.start();

  HC12ReadBuffer.reserve(64);
  HC12.begin(9600);

	Serial.println("Connecting to WiFi");
	WiFi.begin(WIFI_CLIENTSSID, WIFI_CLIENTPASSWORD);
	while(WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println();
	Serial.print("IP Address: ");
	Serial.println(WiFi.localIP());

  Serial.println("Done with setup, entering main loop");
}

void loop()
{
	uptimeTicker.update();

  while (HC12.available()) {
    HC12ByteIn = HC12.read();
    HC12ReadBuffer += char(HC12ByteIn);
    if (HC12ByteIn == '\n') {
      HC12End = true;
    }
  }

  if (HC12End) {
  	Serial.print("Remote: ");
  	Serial.print(HC12ReadBuffer);

    HC12ReadBuffer = "";
    HC12End = false;
  }
}
