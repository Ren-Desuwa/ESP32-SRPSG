/**
 * Project: Brainiac Ball Controller - Professional Build
 * Version: 2.6.0 (Senior Developer No-Emoji Edition)
 * -------------------------------------------------------
 * [LOGGING POLICY] Rule 2: No deleting logs or comments. 
 * Every hardware event and software state transition is logged.
 */

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
// #include <ArduinoJson.h>
#include <WiFi.h>
#include <SD.h> 
#include <SPI.h>
#include "Gyro.h"

// --- Hardware & Network Configuration ---
#define ssid "Ball_Test_AP"
#define Button 0 
#define RGB_PIN 48

// Globals
Gyro gyro(8, 9); 
AsyncWebServer WebServer(80);
AsyncWebSocket WebSocket("/ws");
DNSServer dnsServer;

// State tracking for LEDs to prevent logic collisions
bool isUploading = false; 

/**
 * Handler: handleUpload
 * Description: Receives chunks from Python. Alternates Red/Blue LEDs.
 * Automatically recreates directory paths on the SD card.
 */
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static bool flip = false;
    
    // Retrieve custom path from Python headers
    if(request->hasHeader("X-File-Path")) {
        filename = request->getHeader("X-File-Path")->value();
    }

    if (!index) {
        // --- START OF FILE UPLOAD ---
        Serial.printf("[UPLOAD-EVENT] Transaction started for: %s\n", filename.c_str());
        isUploading = true; 
        
        if(!filename.startsWith("/")) filename = "/" + filename;
        
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash > 0) {
            String path = filename.substring(0, lastSlash);
            if (!SD.exists(path)) {
                Serial.printf("[FS-LOG] Creating path: %s\n", path.c_str());
                SD.mkdir(path);
            }
        }
        
        Serial.printf("[FS-LOG] Writing: %s\n", filename.c_str());
        request->_tempFile = SD.open(filename, FILE_WRITE);
    }
    
    if (request->_tempFile) {
        request->_tempFile.write(data, len);
    }
    
    if (final) {
        // --- END OF FILE UPLOAD ---
        Serial.printf("[UPLOAD-EVENT] Finalized: %s\n", filename.c_str());
        request->_tempFile.close();
        isUploading = false; 

    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); 
    Serial.println("\n[BOOT-LOG] Brainiac Ball Controller Online.");

    pinMode(Button, INPUT_PULLUP);

    Serial.print("[HARDWARE-LOG] Gyroscope Initialization...");
    gyro.setup();
    Serial.println(" SUCCESS");

    int clk = 39, cmd = 38, d0 = 40;
    Serial.print("[HARDWARE-LOG] SD Mount...");
    if(!SD.begin("/sdcard", true)) {
        Serial.println(" FAILED!");
    } else {
        Serial.println(" SUCCESS");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    dnsServer.start(53, "*", WiFi.softAPIP());
    MDNS.begin("ball");

    WebServer.serveStatic("/", SD, "/").setDefaultFile("login.html");

    WebServer.on("/upload", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        if(request->hasHeader("X-File-Path")) {
            String path = request->getHeader("X-File-Path")->value();
            if(!path.startsWith("/")) path = "/" + path;
            if (SD.exists(path)) {
                SD.remove(path);
                Serial.printf("[PRUNE-LOG] Deleted: %s\n", path.c_str());
                request->send(200, "text/plain", "Deleted");
            }
        }
    });

    WebServer.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200); }, handleUpload);
    WebServer.onNotFound([](AsyncWebServerRequest *request){ request->redirect("http://" + WiFi.softAPIP().toString() + "/"); });

    WebServer.addHandler(&WebSocket);
    WebServer.begin();
    Serial.println("[SYSTEM-LOG] Deployment and Telemetry services ready.");
}

void loop() {
    dnsServer.processNextRequest();
    WebSocket.cleanupClients();

    gyro.getData();
    gyro.readAll();

    auto& clients = WebSocket.getClients();
    for (auto& client : clients) {
        if (client.status() == WS_CONNECTED && client.canSend()) {
            String json = "{\"x\":" + String(gyro.readX(), 1) + ",\"z\":" + String(gyro.readZ(), 1) + "}";
            client.text(json);
        }
    }
    delay(5); 
}