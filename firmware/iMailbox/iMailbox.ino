/*
 * iMailbox
 * ESP8266 controlling NeoPixel strip for lighted mailbox
 */

#include <ESP8266WiFi.h>
#include <Base64.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "localconfig.h"

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
#define SLEEPSECONDS 300
#define RETRIES 30

// EEPROM addresses
#define EEPROM_COLOR 0
#define EEPROM_MODE 4

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

uint32_t setColor = strip.Color(0,64,0);
uint16_t lightReading = 0;
uint8_t batteryStatus = 0;
uint16_t elapsedSeconds = 0;
uint32_t uptime = 0;

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

  DEBUG_PRINTLN();
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("iMailbox starting");

/*
  // read stored color
  EEPROM.begin(8);
  setColor = EEPROMReadlong(EEPROM_COLOR);
  DEBUG_PRINT("Read setColor: ");
  DEBUG_PRINTDEC(setColor);
  DEBUG_PRINTLN();
  if(setColor == 4294967295) {
    DEBUG_PRINTLN("Default color not set, writing default to EEPROM");
    setColor = strip.Color(0,64,0);
    EEPROMWritelong(EEPROM_COLOR, setColor);
  }

  // show color
*/
  DEBUG_PRINTLN("Enabling LED strip");
  strip.begin();
/*
  DEBUG_PRINTLN("Setting LED color");
  colorWipe(setColor, 1);
*/

  // connect to wifi
  DEBUG_PRINTLN("Connecting to WiFi");
  connectWifi(ssid, password);

  delay(1000);

  DEBUG_PRINTLN("Done with setup()");
  LEDOff();

  DEBUG_PRINTLN("Sending initial status");
  sendStatus();
  
//  DEBUG_PRINTLN("Going to sleep in 1 second");
//  delay(1000);
//  ESP.deepSleep(SLEEPSECONDS * 1000000);
}

void loop() {
  
  uint16_t i, j, k;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    for(k=0; k < 10; k++) {
      uptimeTick();
      if(elapsedSeconds++ >= 60) {
        sendStatus();
        elapsedSeconds = 0;
      }
      delay(1000);
    }
  }
}

void uptimeTick() {
  uptime++;
}

void sendStatus() {
  LEDOn();
  // get light reading
  DEBUG_PRINT("sendStatus() at uptime of ");
  DEBUG_PRINTDEC(uptime);
  DEBUG_PRINTLN(" seconds");
  DEBUG_PRINTLN("Getting light reading");
  lightReading = analogRead(A0);
  delay(10);
  DEBUG_PRINT("Read raw value: ");
  DEBUG_PRINTDEC(lightReading);
  DEBUG_PRINTLN();

  // get battery status
  DEBUG_PRINTLN("Getting battery status");
  uint8_t battChg = !digitalRead(BATTCHARGEPIN);
  uint8_t battDone = !digitalRead(BATTDONEPIN);
  batteryStatus = battDone + (battChg * 2);
  DEBUG_PRINT("Read raw values CHARGE:");
  DEBUG_PRINTDEC(battChg);
  DEBUG_PRINT(" DONE:");
  DEBUG_PRINTDEC(battDone);
  DEBUG_PRINTLN();

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
      itoa(uptime,value,sizeof(value));
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

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  DEBUG_PRINT("signal strength (RSSI):");
  DEBUG_PRINT(rssi);
  DEBUG_PRINTLN(" dBm");
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

//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to adress + 3.
void EEPROMWritelong(int address, long value) {
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address) {
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
