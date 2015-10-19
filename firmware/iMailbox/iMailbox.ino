/*
 * iMailbox
 * ESP8266 controlling NeoPixel strip for lighted mailbox
 */

#include <ESP8266WiFi.h>
#include <Base64.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "EEPROMAnything.h"
#include "localconfig.h"

// comment out to disable serial debug statements
#define DEBUG

#ifdef DEBUG
 #define DEBUG_PRINT(x)     Serial.print (x)
 #define DEBUG_PRINTDEC(x)     Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x) 
#endif

#define LEDPIN 0
#define NEOPIN 2
#define BATTCHARGEPIN 4
#define BATTDONEPIN 5
#define BATTPGPIN 12
#define SLEEPSECONDS 300
#define RETRIES 30

// EEPROM addresses
#define EEPROM_MODE 0
#define EEPROM_THRESHOLD 1

const char* ssid     = MY_SSID;
const char* password = MY_PWD;

const char server[] = MY_SERVER;
const int serverport = MY_SERVERPORT;

char host[] = MY_HOST;
char base64host[200];
char base64key[200];
char base64value[200];
char key1[] = "lightReading";
char key2[] = "batteryStatus";
char key3[] = "uptime";
char value[10];

WiFiClient client;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, NEOPIN, NEO_GRB + NEO_KHZ800);

uint8_t ledMode = 0;
uint8_t ledShow = 0;
uint16_t lightReading = 0;
uint16_t lightThreshold = 0;
uint8_t batteryStatus = 0;
uint16_t elapsedSeconds = 0;

void setup() {
  delay(500);
  
  Serial.begin(9600);
  delay(10);

  // onboard LED
  pinMode(LEDPIN, OUTPUT);
  LEDOn();
  
  // set battery status pins to input and enable pullup resistors
  pinMode(BATTCHARGEPIN, INPUT_PULLUP);
  pinMode(BATTDONEPIN, INPUT_PULLUP);
  pinMode(BATTPGPIN, INPUT_PULLUP);

  DEBUG_PRINTLN();
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("iMailbox starting");

  // needed on ESP
  EEPROM.begin(512);
  
  // read stored mode
  EEPROM_readAnything(EEPROM_MODE, ledMode);
  DEBUG_PRINT("Read ledMode: ");
  DEBUG_PRINTDEC(ledMode);
  DEBUG_PRINTLN();
  if(ledMode == 255) {
    DEBUG_PRINTLN("Default mode not set, writing default to EEPROM");
    ledMode = 0;
    EEPROM_writeAnything(EEPROM_MODE, ledMode);
  }

  // read stored threshold
  EEPROM_readAnything(EEPROM_THRESHOLD, lightThreshold);
  DEBUG_PRINT("Read lightThreshold: ");
  DEBUG_PRINTDEC(lightThreshold);
  DEBUG_PRINTLN();
  if(lightThreshold == 65535) {
    DEBUG_PRINTLN("Default threshold not set, writing default to EEPROM");
    lightThreshold = 600;
    EEPROM_writeAnything(EEPROM_THRESHOLD, lightThreshold);
  }

  DEBUG_PRINTLN("Updating initial status");
  updateStatus();

  if(batteryStatus == 1) {
    // battery state is low
    DEBUG_PRINTLN("Battery is low!!!");
  }
   
  // enable leds
  DEBUG_PRINTLN("Enabling LED strip");
  strip.begin();
  strip.clear();
  strip.show();

  // connect to wifi
  DEBUG_PRINTLN("Connecting to WiFi");
  connectWifi(ssid, password);

  delay(1000);

  DEBUG_PRINTLN("Done with setup()");
  LEDOff();

  DEBUG_PRINTLN("Sending initial status");
  sendStatus();
}

void loop() {
  
  uint16_t i, j, k;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      if(ledShow == 1) {
        strip.setPixelColor(i, Wheel((i+j) & 255));
      }
    }
    if(ledShow == 1) {
      strip.show();
    }
    for(k=0; k < 10; k++) {
      if(elapsedSeconds++ >= 60) {
        updateStatus();
        if(batteryStatus == 1) {
          // battery state is low
          DEBUG_PRINTLN("Battery is low!!!");
        }
        sendStatus();
        if(ledShow == 0) {
          strip.clear();
          strip.show();
        }
        elapsedSeconds = 0;
      }
      delay(1000);
    }
  }
}

void updateStatus() {
  DEBUG_PRINT("updateStatus() at uptime of ");
  DEBUG_PRINTDEC(millis()/1000);
  DEBUG_PRINTLN(" seconds");
  
  // get light reading
  DEBUG_PRINTLN("Getting light reading");
  lightReading = analogRead(A0);
  delay(10);
  DEBUG_PRINT("lightReading: ");
  DEBUG_PRINTDEC(lightReading);
  DEBUG_PRINT(" lightThreshold: ");
  DEBUG_PRINTDEC(lightThreshold);
  DEBUG_PRINTLN();
  
  if(lightReading >= lightThreshold) {
    DEBUG_PRINTLN("lightReading over threshold, turning on LEDs");  
    ledShow = 1;
  } else {
    DEBUG_PRINTLN("lightReading under threshold, turning off LEDs");
    ledShow = 0;
  }

  // get battery status
  DEBUG_PRINTLN("Getting battery status");
  uint8_t battChg = !digitalRead(BATTCHARGEPIN);
  uint8_t battDone = !digitalRead(BATTDONEPIN);
  uint8_t battPG = !digitalRead(BATTPGPIN);
  batteryStatus = battChg + (battDone * 2) + (battPG * 4);
  DEBUG_PRINT("Read raw values CHARGE:");
  DEBUG_PRINTDEC(battChg);
  DEBUG_PRINT(" DONE:");
  DEBUG_PRINTDEC(battDone);
  DEBUG_PRINT(" POWER GOOD:");
  DEBUG_PRINTDEC(battPG);
  DEBUG_PRINTLN();

  DEBUG_PRINTLN("updateStatus() done");
}

void sendStatus() {
  DEBUG_PRINT("sendStatus() at uptime of ");
  DEBUG_PRINTDEC(millis()/1000);
  DEBUG_PRINTLN(" seconds");
  
  LEDOn();
  
  // send lightReading
  for(int i = 0; i < RETRIES; i++) {
    DEBUG_PRINT("Sending lightReading to Zabbix, try ");
    DEBUG_PRINTDEC(i + 1);
    DEBUG_PRINT(" of ");
    DEBUG_PRINTDEC(RETRIES);
    DEBUG_PRINTLN();
    
    int connectResult = client.connect(server, serverport);
    if(connectResult) {
      DEBUG_PRINTLN("Connected to server");
    
      base64_encode(base64host, host, sizeof(host)-1);
      base64_encode(base64key, key1, sizeof(key1)-1);
      itoa(lightReading,value,sizeof(value));
      base64_encode(base64value, value, sizeof(value)-1);
    
      String s = "<req>\n <host>";
      s += base64host;
      s += "</host>\n <key>";
      s += base64key;
      s += "</key>\n <data>";
      s += base64value;
      s += "</data>\n</req>\n";
      client.print(s);
      delay(10);
      client.stop();
      DEBUG_PRINTLN("lightReading sent");
      break;
    } else {
      delay(1000);
    }
  }

    // send batteryStatus
  for(int i = 0; i < RETRIES; i++) {
    DEBUG_PRINT("Sending batteryStatus to Zabbix, try ");
    DEBUG_PRINTDEC(i + 1);
    DEBUG_PRINT(" of ");
    DEBUG_PRINTDEC(RETRIES);
    DEBUG_PRINTLN();
    
    int connectResult = client.connect(server, serverport);
    if(connectResult) {
      DEBUG_PRINTLN("Connected to server");
    
      base64_encode(base64host, host, sizeof(host)-1);
      base64_encode(base64key, key2, sizeof(key2)-1);
      itoa(batteryStatus,value,sizeof(value));
      base64_encode(base64value, value, sizeof(value)-1);
    
      String s = "<req>\n <host>";
      s += base64host;
      s += "</host>\n <key>";
      s += base64key;
      s += "</key>\n <data>";
      s += base64value;
      s += "</data>\n</req>\n";
      client.print(s);
      delay(10);
      client.stop();
      DEBUG_PRINTLN("batteryStatus sent");
      break;
    } else {
      delay(1000);
    }
  }

  // send uptime
  for(int i = 0; i < RETRIES; i++) {
    DEBUG_PRINT("Sending uptime to Zabbix, try ");
    DEBUG_PRINTDEC(i + 1);
    DEBUG_PRINT(" of ");
    DEBUG_PRINTDEC(RETRIES);
    DEBUG_PRINTLN();
    
    int connectResult = client.connect(server, serverport);
    if(connectResult) {
      DEBUG_PRINTLN("Connected to server");
    
      base64_encode(base64host, host, sizeof(host)-1);
      base64_encode(base64key, key3, sizeof(key3)-1);
      itoa(millis()/1000,value,sizeof(value));
      base64_encode(base64value, value, sizeof(value)-1);
    
      String s = "<req>\n <host>";
      s += base64host;
      s += "</host>\n <key>";
      s += base64key;
      s += "</key>\n <data>";
      s += base64value;
      s += "</data>\n</req>\n";
      client.print(s);
      delay(10);
      client.stop();
      DEBUG_PRINTLN("uptime sent");
      break;
    } else {
      delay(1000);
    }
  }

  LEDOff();

  DEBUG_PRINTLN("sendStatus() done");
}

// Fucntion to connect WiFi
void connectWifi(const char* ssid, const char* password) {
  DEBUG_PRINT("Connecting to ");
  DEBUG_PRINT(ssid);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN("");
  printWifiStatus();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  DEBUG_PRINT("SSID: ");
  DEBUG_PRINTLN(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  DEBUG_PRINT("IP Address: ");
  DEBUG_PRINTLN(ip);
}

void LEDOn() {
  digitalWrite(LEDPIN, 0);
}

void LEDOff() {
  digitalWrite(LEDPIN, 1);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
