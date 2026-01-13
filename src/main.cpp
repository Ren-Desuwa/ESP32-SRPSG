#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "Gyro.h"

#define ssid "ball"
#define Button 19

Gyro gyro(21, 22); // SDA, SCL pins

AsyncWebServer WebServer(80);
AsyncWebSocket WebSocket("/ws");
DNSServer dnsServer;

float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;

// Helper to construct JSON efficiently
String getGyroJson() {
  // Manual string building is much faster than ArduinoJson for high-frequency loops
  String json = "{";
  json += "\"gyroX\":" + String(gyroX, 1); 
  json += ",\"gyroZ\":" + String(gyroZ, 1);
  json += ",\"btn\":" + String(digitalRead(Button) == LOW ? "true" : "false");
  json += "}";
  return json;
}

void setup() {
  Serial.begin(115200);
  pinMode(Button, INPUT_PULLUP);
  gyro.setup();

  // 1. Initialize File System
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Ball_Test_AP");
  WiFi.setSleep(false);

  // DNS Server for Captive Portal
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  if (!MDNS.begin("ball")) {
    Serial.println("Error starting mDNS");
  } else {
    Serial.println("mDNS started: http://ball.local");
    MDNS.addService("http", "tcp", 80);
  }

  WebServer.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  WebServer.on("/handshake", HTTP_POST, [](AsyncWebServerRequest *request){
    // Simple response to say "I'm here"
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // Captive Portal Redirect
  WebServer.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("http://" + WiFi.softAPIP().toString() + "/");
  });

  WebServer.addHandler(&WebSocket);
  WebServer.begin();
  Serial.println("HTTP server started");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // 1. Maintenance
  dnsServer.processNextRequest();
  WebSocket.cleanupClients();

  // 2. Update Sensors (Run as fast as possible)
  gyro.getData();
  gyro.readAll();

  // 3. Send Strategy: "Latest or Nothing"
  // We removed the 'millis()' timer. We send every loop possible.
  
  auto& clients = WebSocket.getClients();
  
  for (auto& client : clients) {
    // Check if the specific client is ready for more data
    // If client.canSend() is FALSE, we simply SKIP this loop. 
    // We don't queue the old data (dropping frames = lower latency).
    if (client.status() == WS_CONNECTED && client.canSend()) {
      
      // Construct JSON on the fly (only when we know we can send)
      String json = "{";
      json += "\"gyroX\":" + String(gyro.readX(), 1); 
      json += ",\"gyroZ\":" + String(gyro.readZ(), 1);
      json += ",\"btn\":" + String(digitalRead(Button) == LOW ? "true" : "false");
      json += "}";
      
      client.text(json);
    }
  }

  // 4. Critical Yield
  // We need at least 1-2ms for the ESP32 WiFi stack to actually transmit the packet
  // If you remove this, the stack chokes.
  delay(5); 
}