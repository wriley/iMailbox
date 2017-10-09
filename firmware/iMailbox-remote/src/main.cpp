/*

Arduino Pro Mini connections
2 NeoPixel
3 Battery charge
4 Battery Done
5 Battery Power Good

6 HC-12 TX
7 HC-12 RX
8 aux input (mailbox door?)
9 HC-12 Set

14 Photocell (ADC)

*/

#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <SoftwareSerial.h>
#include <LowPower.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIXELGPIO 2
#define BATTCHARGEGPIO 3
#define BATTDONEGPIO 4
#define BATTPGGPIO 5
#define HC12TXGPIO 6
#define HC12RXGPIO 7
#define AUXINGPIO 8
#define HC12SETGPIO 9
#define ONEWIREGPIO 13
#define LIGHTLEVELGPIO 14

#define NUMBER_OF_PIXELS 12


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_PIXELS, PIXELGPIO, NEO_GRB + NEO_KHZ800);
SoftwareSerial HC12(HC12TXGPIO, HC12RXGPIO);

OneWire oneWire(ONEWIREGPIO);
DallasTemperature sensors(&oneWire);

byte ledState = HIGH;
unsigned long timer = millis();

char HC12ByteIn;
char SerialByteIn;
String HC12ReadBuffer = "";
bool HC12End = false;
String SerialReadBuffer = "";
bool SerialEnd = false;
uint8_t cmdPixel;
uint8_t cmdLEDR = 0;
uint8_t cmdLEDG = 255;
uint8_t cmdLEDB = 0;
bool runRainbow = false;
int8_t currentRainbow = 0;
bool isDark = false;
int8_t darkReadings = 0;
bool modeNotOff = false;
bool isDisabled = false;
bool isFirstStatus = false;
float tempF;
Ticker statusTicker;
Ticker uptimeTicker;
Ticker rainbowTicker;
bool justWoke = false;

// structs and enums
// must match between base and remote
struct iMailboxStatus {
    uint32_t uptimeSeconds;
    uint32_t colorSingle;
    uint32_t colorFade1;
    uint32_t colorFade2;
    uint16_t lightReading;
    uint16_t lightThreshold;
    uint16_t ambientTemp;
    uint8_t auxInput;
    uint8_t ledMode;
    uint8_t ledShow;
    uint8_t batteryStatus;
    uint8_t brightness;
};

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

// end structs and enums

// custom typed global variables
iMailboxStatus myStatus;
iMailboxStatus myStatusSet;

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

void updateBatteryStatus() {
     uint8_t battChg = !digitalRead(BATTCHARGEGPIO);
     uint8_t battDone = !digitalRead(BATTDONEGPIO);
     uint8_t battPG = !digitalRead(BATTPGGPIO);
     myStatus.batteryStatus = battChg + (battDone * 2) + (battPG * 4);
     Serial.print("batteryStatus: ");
     Serial.println(myStatus.batteryStatus);
}

void updateLightReading() {
    myStatus.lightReading = analogRead(LIGHTLEVELGPIO);
    Serial.print("lightReading: ");
    Serial.println(myStatus.lightReading);
}

void updateAuxInputStatus() {
    myStatus.auxInput = !digitalRead(AUXINGPIO);
    Serial.print("auxInput: ");
    Serial.println(myStatus.auxInput);
}

void updateAmbientTemp() {
    Serial.print("Requesting temperature...");
    sensors.requestTemperatures();
    Serial.println("DONE");

    Serial.print("Temperature for the device 1 (index 0) is: ");
    tempF = DallasTemperature::toFahrenheit(sensors.getTempCByIndex(0));
    myStatus.ambientTemp = (uint16_t)(tempF * 100);
    Serial.print(tempF);
    Serial.println("F");
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

void dumpStatus() {
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

    Serial.print("SD* auxInput: ");
    Serial.println(myStatus.auxInput);

    Serial.print("SD* ambientTemp: ");
    Serial.println(myStatus.ambientTemp);

    Serial.println();
}

void dumpStatusSet() {
    Serial.println("STATUS DUMP:");

    Serial.print("SD* uptimeSeconds: ");
    Serial.println(myStatusSet.uptimeSeconds);

    Serial.print("SD* colorSingle: ");
    Serial.println(myStatusSet.colorSingle);

    Serial.print("SD* colorFade1: ");
    Serial.println(myStatusSet.colorFade1);

    Serial.print("SD* colorFade2: ");
    Serial.println(myStatusSet.colorFade2);

    Serial.print("SD* lightReading: ");
    Serial.println(myStatusSet.lightReading);

    Serial.print("SD* lightThreshold: ");
    Serial.println(myStatusSet.lightThreshold);

    Serial.print("SD* ledMode: ");
    Serial.println(myStatusSet.ledMode);

    Serial.print("SD* ledShow: ");
    Serial.println(myStatusSet.ledShow);

    Serial.print("SD* batteryStatus: ");
    Serial.println(myStatusSet.batteryStatus);

    Serial.print("SD* brightness: ");
    Serial.println(myStatusSet.brightness);

    Serial.print("SD* auxInput: ");
    Serial.println(myStatusSet.auxInput);

    Serial.print("SD* ambientTemp: ");
    Serial.println(myStatusSet.ambientTemp);

    Serial.println();
}

void sendStatus() {
    Serial.println("Sending status to base station");
    dumpStatus();
    HC12.print("SS");
    HC12.write((char *)&myStatus, sizeof(iMailboxStatus));
    HC12.println();
}

void requestStatus() {
    Serial.println("Requesting status from base station");
    HC12.println("RS");
}

void checkDark() {
    if(myStatus.lightReading > myStatus.lightThreshold) {
        darkReadings++;
        if(darkReadings >= 5) {
            isDark = true;
            darkReadings = 5;
        }
    } else {
        darkReadings--;
        if(darkReadings <= 0) {
            isDark = false;
            darkReadings = 0;
        }
    }

    Serial.print("darkReadings: ");
    Serial.print(darkReadings);
    Serial.print("  isDark: ");
    Serial.println(isDark);
}

void statusCB() {
    updateBatteryStatus();
    updateLightReading();
    updateAuxInputStatus();
    updateAmbientTemp();

    checkDark();

    sendStatus();
}

void toggleLED() {
    if(ledState == LOW)
    ledState = HIGH;
    else
    ledState = LOW;
    digitalWrite(LED_BUILTIN, ledState);
}

void incrementUptime() {
    myStatus.uptimeSeconds++;
    toggleLED();
}

void setAllPixels(byte r, byte g, byte b) {
    for(int i = 0; i < NUMBER_OF_PIXELS; i++) {
    strip.setPixelColor(i, r, g, b);
    }
}

void setAllPixels(uint32_t c) {
    for(int i = 0; i < NUMBER_OF_PIXELS; i++) {
    strip.setPixelColor(i, c);
    }
}

void updateRainbow() {
    setAllPixels(Wheel(currentRainbow));
    strip.show();
    currentRainbow++;
}

void setFromStatus() {
    modeNotOff = true;
    runRainbow = false;
    switch(myStatus.ledMode) {
    case OFF:
    modeNotOff = false;
    strip.clear();
    strip.show();
    break;
    case SINGLECOLOR:
    setAllPixels(myStatus.colorSingle);
    strip.show();
    break;
    case RGBFADE:
    runRainbow = true;
    break;
    }

    strip.setBrightness(myStatus.brightness);
}

// main setup
void setup() {
    Serial.begin(9600);
    delay(100);
    Serial.println();
    Serial.println();
    Serial.println("Begin setup");

    HC12ReadBuffer.reserve(64);
    SerialReadBuffer.reserve(64);

    pinMode(LED_BUILTIN, OUTPUT);

    // setup GPIO inputs
    pinMode(BATTCHARGEGPIO, INPUT_PULLUP);
    pinMode(BATTDONEGPIO, INPUT_PULLUP);
    pinMode(BATTPGGPIO, INPUT_PULLUP);
    pinMode(AUXINGPIO, INPUT_PULLUP);
    // setup GPIO outputs
  	pinMode(HC12SETGPIO, OUTPUT);
  	pinMode(PIXELGPIO, OUTPUT);

    digitalWrite(LED_BUILTIN, ledState);

    digitalWrite(HC12SETGPIO, HIGH);
    delay(80);
    HC12.begin(9600);

    myStatus.uptimeSeconds = 0;
    myStatus.colorSingle = 0x0000ff00;
    myStatus.colorFade1 = 0;
    myStatus.colorFade2 = 0;
    myStatus.lightReading = 0;
    myStatus.lightThreshold = 400;
    myStatus.ledMode = RGBFADE;
    myStatus.ledShow = 0;
    myStatus.batteryStatus = 0;
    myStatus.brightness = 63;

    statusTicker.setCallback(statusCB);
    statusTicker.setInterval(60000);
    statusTicker.start();

    uptimeTicker.setCallback(incrementUptime);
    uptimeTicker.setInterval(1000);
    uptimeTicker.start();

    rainbowTicker.setCallback(updateRainbow);
    rainbowTicker.setInterval(200);

    sensors.begin();

    strip.begin();
    strip.show();

    updateBatteryStatus();
    updateLightReading();
    updateAuxInputStatus();
    updateAmbientTemp();

    // we just started up, is it dark out?
    if(myStatus.lightReading > myStatus.lightThreshold) {
        isDark = true;
        darkReadings = 5;
    }

    setFromStatus();
    sendStatus();

    //dumpStatus();

  Serial.println("Setup done, entering main loop");
}

// main loop
void loop() {
    // it's light out and light sensor is not overridden so let's sleep
    if(!isDark && myStatus.ledShow == 0) {
        Serial.println("Going to sleep");
        strip.clear();
        strip.show();
        delay(1000);

        // 8s is max for low.power lib, but with a 75 loops ~ 10mins
        for (int i = 0; i < 75; i++) {
        // Enter power down state for 8 s with ADC and BOD module disabled
            LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
            myStatus.uptimeSeconds+=8; // increment uptime for lost time during sleep
        }

        justWoke = true;
        Serial.println("Woke up, fetching status from base");
        updateLightReading();
        checkDark();
        requestStatus();
        // give base time to respond
        delay(1000);
    }

    statusTicker.update();
    uptimeTicker.update();
    rainbowTicker.update();

    // some serial stuff borrowed from https://www.allaboutcircuits.com/projects/understanding-and-implementing-the-hc-12-wireless-transceiver-module/

    while (HC12.available()) {
        HC12ByteIn = HC12.read();
        HC12ReadBuffer += char(HC12ByteIn);
        if (HC12ByteIn == '\n') {
            HC12End = true;
        }
    }

    while (Serial.available()) {
        SerialByteIn = Serial.read();
        SerialReadBuffer += char(SerialByteIn);
        if (SerialByteIn == '\n') {
            SerialEnd = true;
        }
    }

    if (SerialEnd) {
        if (SerialReadBuffer.startsWith("AT")) {
            HC12.print(SerialReadBuffer);
            delay(100);
            digitalWrite(HC12SETGPIO, LOW);
            delay(100);
            Serial.print(SerialReadBuffer);
            HC12.print(SerialReadBuffer);
            delay(500);
            digitalWrite(HC12SETGPIO, HIGH);
            delay(100);
        } else {
            HC12.print(SerialReadBuffer);
        }
        SerialReadBuffer = "";
        SerialEnd = false;
    }

    if (HC12End) {
        if (HC12ReadBuffer.startsWith("AT")) {
            digitalWrite(HC12SETGPIO, LOW);
            delay(100);
            HC12.print(HC12ReadBuffer);
            delay(500);
            digitalWrite(HC12SETGPIO, HIGH);
            delay(100);
        }

        if (HC12ReadBuffer.startsWith("SS")) {
            Serial.println("Received status from base");
            HC12ReadBuffer.trim();
            byte buf[sizeof(iMailboxStatus)];
            for(uint8_t i = 0; i < sizeof(iMailboxStatus); i++) {
                buf[i] = HC12ReadBuffer[i+2];
            }
            memcpy((void *)&myStatusSet, buf, sizeof(iMailboxStatus));
            myStatus.ledMode = myStatusSet.ledMode;
            myStatus.colorSingle = myStatusSet.colorSingle;
            myStatus.ledShow = myStatusSet.ledShow;
            myStatus.brightness = myStatusSet.brightness;
            myStatus.lightThreshold = myStatusSet.lightThreshold;
            setFromStatus();
            if(isFirstStatus) {
                isFirstStatus = false;
                sendStatus();
            }
            dumpStatusSet();
            Serial.println("My status");
            dumpStatus();
        }

        if (HC12ReadBuffer.startsWith("RS")) {
            Serial.println("Got status request from base");
            sendStatus();
        }

        HC12ReadBuffer = "";
        HC12End = false;
    }

    if(justWoke) {
        justWoke = false;
        sendStatus();
    }

    // Control logic
    if(isDark || myStatus.ledShow == 1) {
        if(isDisabled) {
            Serial.println("Re-enabling!");
            isDisabled = false;
        }

        if(runRainbow) {
            if(rainbowTicker.state() == STOPPED) {
                rainbowTicker.start();
            }
        } else {
            rainbowTicker.stop();
        }
    } else {
        if(rainbowTicker.state() != STOPPED) {
            rainbowTicker.stop();
        }
        if(!isDisabled) {
            Serial.println("Disabling!");
            setAllPixels(0);
            strip.show();
            isDisabled = true;
        }
    }
}
