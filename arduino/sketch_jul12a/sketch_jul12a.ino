/*
 *  Simple HTTP get webclient test
 */

#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

#define DEBUG 1

#define PRINTDEBUG(STR) \
  {	\
    if (DEBUG) Serial.println(STR); \
  }

#define LEDPIN 0
#define NEOPIN 2
#define BATTCHARGEPIN 4
#define BATTDONEPIN 5

#define SLEEPSECONDS 30

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, NEOPIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

#include "localconfig.h"
const char* ssid     = MY_SSID;
const char* password = MY_PWD;

WiFiServer server(80);

// Fucntion to connect WiFi
void connectWifi(const char* ssid, const char* password) {
  int WiFiCounter = 0;
  // We start by connecting to a WiFi network
  PRINTDEBUG("Connecting to ");
  PRINTDEBUG(ssid);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && WiFiCounter < 30) {
    delay(1000);
    WiFiCounter++;
    PRINTDEBUG(".");
  }

  PRINTDEBUG("");
  PRINTDEBUG("WiFi connected");
  PRINTDEBUG("IP address: ");
  PRINTDEBUG(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);
  delay(10);

  // onboard LED
  pinMode(LEDPIN, OUTPUT);
  
  // set battery status pins to input and enable pullup resistors
  pinMode(BATTCHARGEPIN, INPUT);
  digitalWrite(BATTCHARGEPIN, HIGH);
  pinMode(BATTDOENPIN, INPUT);
  digitalWrite(BATTDONEPIN, HIGH);

  Serial.println();
  Serial.println();
  Serial.println("iMailbox v0.01");
  
  connectWifi(ssid, password); 
  
  server.begin();

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  delay(1000);
  
  rainbowCycle(255);
  
  //PRINTDEBUG("Going to sleep for SLEEPSECONDS seconds");
  //delay(5000);
  //ESP.deepSleep(SLEEPSECONDS * 1000000);
}

int value = 0;

void loop() {
	delay(10);
	WiFiClient clientS = server.available();
	if(clientS) {
		PRINTDEBUG("new client");
		while(!clientS.available()) {}
		String req = clientS.readStringUntil('\r');
		PRINTDEBUG(req);
		clientS.flush();
		
		String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n<body>\r\n";
		if(req.indexOf("/status") != -1) {
			s += "analog: ";
			s += String(analogRead(A0));
			s += "\r\n<br>\r\nbattery charge: ";
			s += String(digitalRead(BATTCHARGEPIN));
			s += "\r\n<br>\r\nbattery done: ";
			s += String(digitalRead(BATTDONEPIN));
		} else {
			s += "iMailbox (use /status)"
		}
		
		s += "\r\n</body>\r\n</html>\r\n";
		
		clientS.print(s);
		delay(10);
		clientS.stop();
	}
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
