/*

Arduino Pro Mini connections
2 NeoPixel

6 HC-12 TX
7 HC-12 RX
9 HC-12 Set


*/

#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <SoftwareSerial.h>

const byte HC12SetPin = 9;
const byte pixelPin = 2;
const byte numPixels = 12;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, pixelPin, NEO_GRB + NEO_KHZ800);
SoftwareSerial HC12(6, 7);

byte ledState = HIGH;
unsigned long timer = millis();                 // Delay Timer
char HC12ByteIn;                                // Temporary variable
String HC12ReadBuffer = "";                     // Read/Write Buffer 1 for HC12
boolean HC12End = false;                        // Flag to indiacte End of HC12 String
boolean commandMode = false;                    // Send AT commands
uint8_t cmdPixel;
uint8_t cmdLEDR = 0;
uint8_t cmdLEDG = 255;
uint8_t cmdLEDB = 0;
bool runRainbow = false;
byte currentRainbow = 0;

Ticker statusTicker;
Ticker uptimeTicker;
Ticker rainbowTicker;

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

iMailboxStatus myStatus;

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

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (uint8_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, c);  //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint8_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (uint8_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint8_t i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

void DumpStatus() {
	Serial.println("STATUS DUMP:");

	Serial.print("SD* uptimeSeconds: ");
	Serial.println(myStatus.uptimeSeconds);

	Serial.print("SD* colorSingle: ");
	Serial.println(myStatus.colorSingle);

	Serial.print("SD* colorFade1: ");
	Serial.println(myStatus.colorFade1);

	Serial.print("SD* colorFade2: ");
	Serial.println(myStatus.colorFade2);

	Serial.print("SD* lightReading: ");
	Serial.println(myStatus.lightReading);

	Serial.print("SD* lightThreshold: ");
	Serial.println(myStatus.lightThreshold);

	Serial.print("SD* ledMode: ");
	Serial.println(myStatus.ledMode);

	Serial.print("SD* ledShow: ");
	Serial.println(myStatus.ledShow);

	Serial.print("SD* batteryStatus: ");
	Serial.println(myStatus.batteryStatus);

	Serial.print("SD* brightness: ");
	Serial.println(myStatus.brightness);
}

void SendStatus() {
  HC12.print("SS");
  HC12.write((char *)&myStatus, sizeof(iMailboxStatus));
  HC12.println();
}

void StatusCB() {
	Serial.println("Sending status to base station");
	DumpStatus();
	SendStatus();
	Serial.println("Requesting status from base station");
	HC12.println("RS");
}

void toggleLED() {
	if(ledState == LOW)
		ledState = HIGH;
	else
		ledState = LOW;
	digitalWrite(LED_BUILTIN, ledState);
}

void IncrementUptime() {
  myStatus.uptimeSeconds++;
	toggleLED();
}

void SetAllPixels(byte r, byte g, byte b) {
	for(int i = 0; i < numPixels; i++) {
		strip.setPixelColor(i, r, g, b);
	}
}

void SetAllPixels(uint32_t c) {
	for(int i = 0; i < numPixels; i++) {
		strip.setPixelColor(i, c);
	}
}

void UpdateRainbow() {
	SetAllPixels(Wheel(currentRainbow));
	strip.show();
	currentRainbow++;
}

void SetFromStatus() {
	switch(myStatus.ledMode) {
		case OFF:
			SetAllPixels(strip.Color(0, 0, 0));
			strip.show();
			break;
		case SINGLECOLOR:
			SetAllPixels(myStatus.colorSingle);
			strip.show();
			break;
		case RGBFADE:
			runRainbow = true;
			break;
	}

	strip.setBrightness(myStatus.brightness);
}

void setup() {
  Serial.begin(115200);
  delay(100);
	Serial.println();
	Serial.println();
  Serial.println("Begin setup");

  HC12ReadBuffer.reserve(64);

	pinMode(1, OUTPUT);	// Built in LED on DigiSpark Pro
	digitalWrite(1, ledState);

  pinMode(HC12SetPin, OUTPUT);                  // Output High for Transparent / Low for Command
  pinMode(pixelPin, OUTPUT);
  digitalWrite(HC12SetPin, HIGH);               // Enter Transparent mode
  delay(80);                                    // 80 ms delay before operation per datasheet
  HC12.begin(9600);                             // Open software serial port to HC12

  myStatus.uptimeSeconds = 0;
	myStatus.colorSingle = 0x0000ff00;
	myStatus.colorFade1 = 0;
	myStatus.colorFade2 = 0;
	myStatus.lightReading = 0;
	myStatus.lightThreshold = 400;
	myStatus.ledMode = SINGLECOLOR;
	myStatus.ledShow = 0;
	myStatus.batteryStatus = 0;
	myStatus.brightness = 63;

  statusTicker.setCallback(StatusCB);
  statusTicker.setInterval(60000);
  statusTicker.start();

  uptimeTicker.setCallback(IncrementUptime);
  uptimeTicker.setInterval(1000);
  uptimeTicker.start();

	rainbowTicker.setCallback(UpdateRainbow);
  rainbowTicker.setInterval(200);

  strip.begin();
  strip.show();

	SetFromStatus();

	SendStatus();

  Serial.println("Setup done, entering main loop");
}

void loop() {
  statusTicker.update();
  uptimeTicker.update();
	rainbowTicker.update();

  while (HC12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = HC12.read();                   // Store each character from rx buffer in byteIn
    HC12ReadBuffer += char(HC12ByteIn);         // Write each character of byteIn to HC12ReadBuffer
    if (HC12ByteIn == '\n') {                   // At the end of the line
      HC12End = true;                           // Set HC12End flag to true
    }
  }

  if (HC12End) {                                // If HC12End flag is true
    if (HC12ReadBuffer.startsWith("AT")) {      // Check to see if a command is received from remote
      digitalWrite(HC12SetPin, LOW);            // Enter command mode
      delay(100);                               // Delay before sending command
			HC12.print(HC12ReadBuffer);               // Write command to local HC12
      delay(500);                               // Wait 0.5 s for reply
      digitalWrite(HC12SetPin, HIGH);           // Exit command / enter transparent mode
      delay(100);                               // Delay before proceeding
    }

    if (HC12ReadBuffer.startsWith("SS")) {
			Serial.println("Got Status");
			HC12ReadBuffer.trim();
			byte buf[sizeof(iMailboxStatus)];
			for(uint8_t i = 0; i < sizeof(iMailboxStatus); i++) {
				buf[i] = HC12ReadBuffer[i+2];
			}
			uint32_t temp = myStatus.uptimeSeconds;
			memcpy((void *)&myStatus, buf, sizeof(iMailboxStatus));
			myStatus.uptimeSeconds = temp;
			SetFromStatus();
			DumpStatus();
		}

		if (HC12ReadBuffer.startsWith("RS")) {
			Serial.println("Got Status Request");
			SendStatus();
		}

    HC12ReadBuffer = "";                        // Empty buffer
    HC12End = false;                            // Reset flag
  }

	if(runRainbow) {
		if(rainbowTicker.state() == STOPPED) {
			Serial.println("starting rainbow");
			rainbowTicker.start();
		}
	} else {
		if(rainbowTicker.state() == RUNNING) {
			Serial.println("stopping rainbow");
			rainbowTicker.stop();
		}
	}
}
