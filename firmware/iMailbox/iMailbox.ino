/*
 * iMailbox
 * ESP8266 controlling NeoPixel strip for lighted mailbox
 */

#include <ESP8266WiFi.h>
#include <Base64.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "localconfig.h"

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
char key1[] = "lightReading";
char base64key1[200];
char key2[] = "batteryStatus";
char base64key2[200];
char value1[10];
char base64value1[200];
char value2[10];
char base64value2[200];


WiFiClient client;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, NEOPIN, NEO_GRB + NEO_KHZ800);

uint32_t setColor = strip.Color(0,64,0);
int lightReading = 0;
uint8_t batteryStatus = 0;

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

  Serial.println();
  Serial.println();
  Serial.println("iMailbox starting");

  // read stored color
  EEPROM.begin(8);
  setColor = EEPROMReadlong(EEPROM_COLOR);
  Serial.print("Read setColor: ");
  Serial.println(setColor, DEC);
  if(setColor == 4294967295) {
    Serial.println("lastColor not set, writing default to EEPROM");
    setColor = strip.Color(0,64,0);
    EEPROMWritelong(EEPROM_COLOR, setColor);
  }

  // show color
  Serial.println("Enabling LED strip");
  strip.begin();
  Serial.println("Setting LED color");
  colorWipe(setColor, 1);

  // get light reading
  Serial.println("Getting light reading");
  lightReading = analogRead(A0);
  Serial.print("Read raw value: ");
  Serial.print(lightReading, DEC);
  Serial.println();

  // get battery status
  Serial.println("Getting battery status");
  uint8_t battChg = !digitalRead(BATTCHARGEPIN);
  uint8_t battDone = !digitalRead(BATTDONEPIN);
  batteryStatus = battDone + (battChg * 2);
  Serial.print("Read raw values CHARGE:");
  Serial.print(battChg, DEC);
  Serial.print(" DONE:");
  Serial.print(battDone, DEC);
  Serial.println();

  // connect to wifi
  Serial.println("Connecting to WiFi");
  connectWifi(ssid, password);

  delay(1000);
  
  // send lightReading
  for(int i = 0; i < RETRIES; i++) {
    Serial.print("Sending lightReading to Zabbix, try ");
    Serial.print(i + 1, DEC);
    Serial.print(" of ");
    Serial.println(RETRIES, DEC);
    
    int connectResult = client.connect(server, serverport);
    if(connectResult) {
      Serial.println("Connected to server");
	  
      base64_encode(base64host, host, sizeof(host)-1);
	    base64_encode(base64key1, key1, sizeof(key1)-1);
      itoa(lightReading,value1,sizeof(value1));
	    base64_encode(base64value1, value1, sizeof(value1)-1);
	  
  	  String s = "<req>\n <host>";
  	  s += base64host;
  	  s += "</host>\n <key>";
  	  s += base64key1;
  	  s += "</key>\n <data>";
  	  s += base64value1;
  	  s += "</data>\n</req>\n";
  	  client.print(s);
  	  delay(10);
      client.stop();
      Serial.println("lightReading sent");
      break;
    } else {
      delay(1000);
    }
  }

    // send batteryStatus
  for(int i = 0; i < RETRIES; i++) {
    Serial.print("Sending batteryStatus to Zabbix, try ");
    Serial.print(i + 1, DEC);
    Serial.print(" of ");
    Serial.println(RETRIES, DEC);
    
    int connectResult = client.connect(server, serverport);
    if(connectResult) {
      Serial.println("Connected to server");
    
      base64_encode(base64host, host, sizeof(host)-1);
      base64_encode(base64key2, key2, sizeof(key2)-1);
      itoa(batteryStatus,value2,sizeof(value2));
      base64_encode(base64value2, value2, sizeof(value2)-1);
    
      String s = "<req>\n <host>";
      s += base64host;
      s += "</host>\n <key>";
      s += base64key2;
      s += "</key>\n <data>";
      s += base64value2;
      s += "</data>\n</req>\n";
      client.print(s);
      delay(10);
      client.stop();
      Serial.println("batteryStatus sent");
      break;
    } else {
      delay(1000);
    }
  }

  //
  // disconnect from wifi
  // ******* This is disabled because it causes a reset **************
//  Serial.println("Disconnecting WiFi");
//  WiFi.disconnect();

  Serial.println("Done with setup()");
  LEDOff();
  
  Serial.println("Going to sleep in 1 second");
  delay(1000);
  ESP.deepSleep(SLEEPSECONDS * 1000000);
}

void loop() {
}

// Fucntion to connect WiFi
void connectWifi(const char* ssid, const char* password) {
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  printWifiStatus();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void LEDOn() {
  digitalWrite(LEDPIN, 0);
}

void LEDOff() {
  digitalWrite(LEDPIN, 1);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
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
