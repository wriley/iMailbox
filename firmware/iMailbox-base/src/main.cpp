#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <Ticker.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "config.h"

SoftwareSerial HC12(14, 12);

char HC12ByteIn;
String HC12ReadBuffer = "";
boolean HC12End = false;
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
	uint8_t dummy;
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

void DumpStatusSet() {
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

void SendStatus() {
  HC12.print("SS");
  HC12.write((char *)&remoteStatusSet, sizeof(iMailboxStatus));
  HC12.println();
}

void RequestStatus() {
	HC12.println("RS");
}

String GetContentType(String filename) {
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

String FormatBytes(size_t bytes){
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

bool HandleFileRead(String path) {
	if (path.endsWith("/"))
		path += "index.html";
	String contentType = GetContentType(path);
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

void HandleFileUpload(){
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

void HandleFileDelete(){
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

void HandleFileCreate(){
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

void HandleFileList() {
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

void HandleNotFound(){
	if(!HandleFileRead(httpServer.uri())) {
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

void HandleStatus() {
	StaticJsonBuffer<400> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["ledMode"] = remoteStatus.ledMode;
	root["ledShow"] = remoteStatus.ledShow;
	root["brightness"] = remoteStatus.brightness;
	root["lightReading"] = remoteStatus.lightReading;
	root["lightThreshold"] = remoteStatus.lightThreshold;
	root["batteryStatus"] = remoteStatus.batteryStatus;
	root["uptimeSeconds"] = remoteStatus.uptimeSeconds;
	root["uptimeSecondsBase"] = uptimeSeconds;
	root["freeHeap"] = ESP.getFreeHeap();
	root["lastStatus"] = millis() - lastStatus;
	char msg[400];
	char *firstChar = msg;
	root.printTo(firstChar, sizeof(msg) - strlen(msg));
	httpServer.send(200, "text/json", msg);
}

void Send302(String location) {
	httpServer.sendHeader("Location", location, true);
	httpServer.send(302, "text/plain", "");
	httpServer.client().stop();
}

void HandleSet() {
	if(httpServer.uri() != "/set.cgi") return;
	for(int i = 0; i < httpServer.args(); i++) {
		String arg = httpServer.argName(i);
		Serial.print("Processing arg ");
		Serial.println(arg);
		if(arg == "ledMode") {
			uint8_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			remoteStatusSet.ledMode = val;
			remoteStatus.ledMode = val;
		} else if (arg == "currentColor") {
			uint32_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			remoteStatusSet.colorSingle = val;
			remoteStatus.colorSingle = val;
		} else if (arg == "ledShow") {
			uint8_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			remoteStatusSet.ledShow = val;
			remoteStatus.ledShow = val;
		} else if (arg == "brightness") {
			uint16_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			remoteStatusSet.brightness = val;
			remoteStatus.brightness = val;
		} else if (arg == "lightThreshold") {
			uint16_t val = httpServer.arg(i).toInt();
			Serial.print("Got value: ");
			Serial.println(val);
			remoteStatusSet.lightThreshold = val;
			remoteStatus.lightThreshold = val;
		}
		SendStatus();
	}
	Send302("/");
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

	Serial.println("Connecting to WiFi");
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
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), FormatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }

	httpUpdater.setup(&httpServer, UPDATE_PATH, UPDATE_USERNAME, UPDATE_PASSWORD);

	httpServer.on("/status", HTTP_GET, HandleStatus);
	httpServer.on("/list", HTTP_GET, HandleFileList);
	httpServer.on("/edit", HTTP_GET, [](){
    if(!HandleFileRead("/edit.html")) httpServer.send(404, "text/plain", "FileNotFound");
  });
	httpServer.on("/edit", HTTP_PUT, HandleFileCreate);
	httpServer.on("/edit", HTTP_DELETE, HandleFileDelete);
	httpServer.on("/edit", HTTP_POST, [](){ httpServer.send(200, "text/plain", ""); }, HandleFileUpload);
	httpServer.on("/set.cgi", HTTP_POST, HandleSet);

	httpServer.onNotFound(HandleNotFound);

  httpServer.begin();

  Serial.println("Done with setup, entering main loop");

	DumpStatusSet();
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
			SendStatus();
		}

    HC12ReadBuffer = "";
    HC12End = false;
  }
}