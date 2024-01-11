#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>

#define ONE_WIRE_BUS 4 // Change this to the pin where your Dallas sensor is connected
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Replace with your network credentials
const char* ssid = "Idkyet";
const char* password = "euz38zqe";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3000;

// Init BME280
void initDallas() {
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  sensors.begin();
}

// Get Sensor Readings and return JSON object
String getSensorReadings() {
  sensors.requestTemperatures(); 
  readings["temperature"] = String(sensors.getTempCByIndex(0));
  readings["timestamp"] = String(millis()); // Add current time in milliseconds
  String jsonString = JSON.stringify(readings);
  return jsonString + "\n"; // Add a newline character at the end
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings.c_str());
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    String command(reinterpret_cast<char*>(data), len);
    if (command == "getReadings") {
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    }
    // You can add more commands here if needed
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}
void setup() {
  Serial.begin(115200);
  initDallas();
  initWiFi();
  initSPIFFS();
  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = getSensorReadings();
    Serial.print(sensorReadings);
    notifyClients(sensorReadings);
    lastTime = millis();
  }

  ws.cleanupClients();
}
void initSDCard() {
    #define SD_CS_PIN 10

    Serial.println("Initializing SD card...");
    if(!SD.begin(SD_CS_PIN)){
      Serial.println("Card Mount Failed");
      return;
    }

    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
    }


    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    if (!SD.exists("/tempData.csv")) {
        Serial.println("tempData.csv does not exist, creating...");
        File file = SD.open("/tempData.csv", FILE_WRITE);
        if (file) {
            file.println("Timestamp, Temperature(°C)");
            file.close();
            Serial.println("tempData.csv created successfully");
        } else {
            Serial.println("Failed to create tempData.csv");
        }
    } else {
        Serial.println("tempData.csv already exists");
    }
}
