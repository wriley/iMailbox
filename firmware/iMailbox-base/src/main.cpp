/*

NOdeMCU connections
4 	HC-12 set
12 	HC-12 Tx
14 	HC-12 Rx

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <Ticker.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "config.h"

#define HC12RXGPIO 12
#define HC12TXGPIO 14
#define HC12SETGPIO 4

SoftwareSerial HC12(HC12RXGPIO, HC12TXGPIO);

char HC12ByteIn;                                // Temporary variable
char SerialByteIn;
String HC12ReadBuffer = "";                     // Read/Write Buffer 1 for HC12
bool HC12End = false;                        // Flag to indiacte End of HC12 String
String SerialReadBuffer = "";
bool SerialEnd = false;

uint32_t uptimeSeconds;
byte ledState = HIGH;
File fsUploadFile;
uint32_t lastStatus = 0;

WiFiClient espClient;
Ticker uptimeTicker;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

struct __attribute__((aligned(4))) iMailboxStatus {
	uint32_t uptimeSeconds;
	uint32_t colorSingle;
	uint32_t colorFade1;
	uint32_t colorFade2;
	uint16_t lightReading;
	uint16_t lightThreshold;
	uint8_t auxInput;
	uint8_t ledMode;
	uint8_t ledShow;
	uint8_t batteryStatus;
	uint8_t brightness;
};

iMailboxStatus remoteStatus;
iMailboxStatus remoteStatusSet;

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

void incrementUptime() {
  uptimeSeconds++;
	toggleLED();
}

void wifiEvent(WiFiEvent_t event) {
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

void dumpStatus() {
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

void dumpStatusSet() {
	Serial.println("STATUS DUMP:");

	Serial.print("SD* uptimeSeconds: ");
	Serial.println(remoteStatusSet.uptimeSeconds);

	Serial.print("SD* colorSingle: ");
	Serial.println(remoteStatusSet.colorSingle);

	Serial.print("SD* colorFade1: ");
	Serial.println(remoteStatusSet.colorFade1);

	Serial.print("SD* colorFade2: ");
	Serial.println(remoteStatusSet.colorFade2);

	Serial.print("SD* lightReading: ");
	Serial.println(remoteStatusSet.lightReading);

	Serial.print("SD* lightThreshold: ");
	Serial.println(remoteStatusSet.lightThreshold);

	Serial.print("SD* ledMode: ");
	Serial.println(remoteStatusSet.ledMode);

	Serial.print("SD* ledShow: ");
	Serial.println(remoteStatusSet.ledShow);

	Serial.print("SD* batteryStatus: ");
	Serial.println(remoteStatusSet.batteryStatus);

	Serial.print("SD* brightness: ");
	Serial.println(remoteStatusSet.brightness);
}

void sendStatus() {
	Serial.println("Sending status to remote");
	dumpStatusSet();
  HC12.print("SS");
  HC12.write((char *)&remoteStatusSet, sizeof(iMailboxStatus));
  HC12.println();
}

void requestStatus() {
	HC12.println("RS");
}

String getContentType(String filename) {
	if (httpServer.hasArg("download")) return "application/octet-stream";
	else if (filename.endsWith(".htm")) return "text/html";
	else if (filename.endsWith(".html")) return "text/html";
	else if (filename.endsWith(".css")) return "text/css";
	else if (filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".json")) return "application/json";
	else if (filename.endsWith(".png")) return "image/png";
	else if (filename.endsWith(".gif")) return "image/gif";
	else if (filename.endsWith(".jpg")) return "image/jpeg";
	else if (filename.endsWith(".ico")) return "image/x-icon";
	else if (filename.endsWith(".xml")) return "text/xml";
	else if (filename.endsWith(".pdf")) return "application/x-pdf";
	else if (filename.endsWith(".zip")) return "application/x-zip";
	else if (filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

bool handleFileRead(String path) {
	if (path.endsWith("/"))
		path += "index.html";
	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";
	if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
		if (SPIFFS.exists(pathWithGz))
			path += ".gz";
		File file = SPIFFS.open(path, "r");
		size_t sent = httpServer.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;
}

void handleFileUpload(){
  if(httpServer.uri() != "/edit") return;
  HTTPUpload& upload = httpServer.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
  }
}

void handleFileDelete(){
  if(httpServer.args() == 0) return httpServer.send(500, "text/plain", "BAD ARGS");
  String path = httpServer.arg(0);
  if(path == "/")
    return httpServer.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return httpServer.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  httpServer.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(httpServer.args() == 0)
    return httpServer.send(500, "text/plain", "BAD ARGS");
  String path = httpServer.arg(0);
  if(path == "/")
    return httpServer.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return httpServer.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return httpServer.send(500, "text/plain", "CREATE FAILED");
  httpServer.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!httpServer.hasArg("dir")) {httpServer.send(500, "text/plain", "BAD ARGS"); return;}

  String path = httpServer.arg("dir");
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  httpServer.send(200, "text/json", output);
}

void handleNotFound(){
	if(!handleFileRead(httpServer.uri())) {
	  String message = "File Not Found\n\n";
	  message += "URI: ";
	  message += httpServer.uri();
	  message += "\nMethod: ";
	  message += (httpServer.method() == HTTP_GET)?"GET":"POST";
	  message += "\nArguments: ";
	  message += httpServer.args();
	  message += "\n";
	  for (uint8_t i=0; i<httpServer.args(); i++){
	    message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
	  }
	  httpServer.send(404, "text/plain", message);
	}
}

void handleStatus() {
	StaticJsonBuffer<400> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["ledMode"] = remoteStatus.ledMode;
	root["ledShow"] = remoteStatus.ledShow;
	root["brightness"] = remoteStatus.brightness;
	root["lightReading"] = remoteStatus.lightReading;
	root["lightThreshold"] = remoteStatus.lightThreshold;
	root["batteryStatus"] = remoteStatus.batteryStatus;
	root["uptimeSeconds"] = remoteStatus.uptimeSeconds;
	root["colorSingle"] = remoteStatus.colorSingle;
	root["uptimeSecondsBase"] = uptimeSeconds;
	root["freeHeap"] = ESP.getFreeHeap();
	root["lastStatus"] = millis() - lastStatus;
	char msg[400];
	char *firstChar = msg;
	root.printTo(firstChar, sizeof(msg) - strlen(msg));
	httpServer.send(200, "text/json", msg);
}

void send302(String location) {
	httpServer.sendHeader("Location", location, true);
	httpServer.send(302, "text/plain", "");
	httpServer.client().stop();
}

void handleSet() {
	if(httpServer.uri() != "/set.cgi") return;
	for(int i = 0; i < httpServer.args(); i++) {
		String arg = httpServer.argName(i);
		Serial.print("Processing arg ");
		Serial.println(arg);
		if(arg == "ledMode") {
			uint8_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			if(val >= 0) {
				remoteStatusSet.ledMode = val;
				remoteStatus.ledMode = val;
			}
		} else if (arg == "currentColor") {
			int32_t val = strtol(httpServer.arg(i).c_str(), 0, 16);
			Serial.print("Got value: ");
			Serial.println(val);
			if(val >= 0) {
				remoteStatusSet.colorSingle = val;
				remoteStatus.colorSingle = val;
			}
		} else if (arg == "ledShow") {
			uint8_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			if(val >= 0) {
				remoteStatusSet.ledShow = val;
				remoteStatus.ledShow = val;
			}
		} else if (arg == "brightness") {
			uint16_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			if(val >= 0) {
				remoteStatusSet.brightness = val;
				remoteStatus.brightness = val;
			}
		} else if (arg == "lightThreshold") {
			uint16_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			if(val >= 0) {
				remoteStatusSet.lightThreshold = val;
				remoteStatus.lightThreshold = val;
			}
		}
	}
	sendStatus();
	send302("/");
}

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("");
  Serial.println("Beginning setup");

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, ledState);

	// setup GPIO inputs

	// setup GPIO outputs
	pinMode(HC12SETGPIO, OUTPUT);

	uptimeTicker.setCallback(incrementUptime);
  uptimeTicker.setInterval(1000);
  uptimeTicker.start();

  HC12ReadBuffer.reserve(64);
  HC12.begin(9600);

	remoteStatusSet.uptimeSeconds = 0;
	remoteStatusSet.colorSingle = 0x00ff0000;
	remoteStatusSet.colorFade1 = 0;
	remoteStatusSet.colorFade2 = 0;
	remoteStatusSet.lightReading = 0;
	remoteStatusSet.lightThreshold = 400;
	remoteStatusSet.ledMode = RGBFADE;
	remoteStatusSet.ledShow = 0;
	remoteStatusSet.batteryStatus = 0;
	remoteStatusSet.brightness = 63;

	Serial.print("Connecting to WiFi: ");
	Serial.println(WIFI_CLIENTSSID);
	WiFi.begin(WIFI_CLIENTSSID, WIFI_CLIENTPASSWORD);
	while(WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println();
	Serial.print("IP Address: ");
	Serial.println(WiFi.localIP());

	bool result = SPIFFS.begin();
 	Serial.println("SPIFFS opened: " + result);
	{
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }

	httpUpdater.setup(&httpServer, UPDATE_PATH, UPDATE_USERNAME, UPDATE_PASSWORD);

	httpServer.on("/status", HTTP_GET, handleStatus);
	httpServer.on("/list", HTTP_GET, handleFileList);
	httpServer.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.html")) httpServer.send(404, "text/plain", "FileNotFound");
  });
	httpServer.on("/edit", HTTP_PUT, handleFileCreate);
	httpServer.on("/edit", HTTP_DELETE, handleFileDelete);
	httpServer.on("/edit", HTTP_POST, [](){ httpServer.send(200, "text/plain", ""); }, handleFileUpload);
	httpServer.on("/set.cgi", HTTP_POST, handleSet);
	httpServer.onNotFound(handleNotFound);

  httpServer.begin();

	requestStatus();
	delay(500);

  Serial.println("Done with setup, entering main loop");

	dumpStatusSet();
}

void loop()
{
	uptimeTicker.update();

	httpServer.handleClient();

  while (HC12.available()) {
    HC12ByteIn = HC12.read();
    HC12ReadBuffer += char(HC12ByteIn);
    if (HC12ByteIn == '\n') {
      HC12End = true;
    }
  }

	while (Serial.available()) {                  // If Arduino's computer rx buffer has data
    SerialByteIn = Serial.read();               // Store each character in byteIn
    SerialReadBuffer += char(SerialByteIn);     // Write each character of byteIn to SerialReadBuffer
    if (SerialByteIn == '\n') {                 // Check to see if at the end of the line
      SerialEnd = true;                         // Set SerialEnd flag to indicate end of line
    }
  }

  if (SerialEnd) {                              // Check to see if SerialEnd flag is true

    if (SerialReadBuffer.startsWith("AT")) {    // Has a command been sent from local computer
      HC12.print(SerialReadBuffer);             // Send local command to remote HC12 before changing settings
      delay(100);                               //
      digitalWrite(HC12SETGPIO, LOW);            // Enter command mode
      delay(100);                               // Allow chip time to enter command mode
      Serial.print(SerialReadBuffer);           // Echo command to serial
      HC12.print(SerialReadBuffer);             // Send command to local HC12
      delay(500);                               // Wait 0.5s for a response
      digitalWrite(HC12SETGPIO, HIGH);           // Exit command / enter transparent mode
      delay(100);                               // Delay before proceeding
    } else {
      HC12.print(SerialReadBuffer);             // Transmit non-command message
    }
    SerialReadBuffer = "";                      // Clear SerialReadBuffer
    SerialEnd = false;                          // Reset serial end of line flag
  }

  if (HC12End) {
		if (HC12ReadBuffer.startsWith("SS")) {
			//Serial.println("Got Status");
			lastStatus = millis();
			HC12ReadBuffer.trim();
			byte buf[sizeof(iMailboxStatus)];
			for(uint8_t i = 0; i < sizeof(iMailboxStatus); i++) {
				buf[i] = HC12ReadBuffer[i+2];
			}
			memcpy((void *)&remoteStatus, buf, sizeof(iMailboxStatus));
		}

		if (HC12ReadBuffer.startsWith("RS")) {
			//Serial.println("Got Status Request");
			sendStatus();
		}

    HC12ReadBuffer = "";
    HC12End = false;
  }
}
