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
#include <ArduinoJson.h>
#include <WiFi.h>
#include "Neo.h"
#include <SD_MMC.h> 
#include <FS.h>
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
 * LOGIC: setNormalShuffle
 * [RULE 2 LOG] Explicitly clearing and rebuilding the color shuffle queue.
 */
void setNormalShuffle() {
    Serial.println("[LED-STATE] Transitioning to Normal Shuffle. Invoking neoClearAll().");
    neoClearAll(); // Clear previous states first
    
    Serial.println("[LED-STATE] Building sequence: Blue -> Green -> Red -> Purple -> Yellow -> Cyan -> White -> Off.");
    
    int c1[3] = {0, 0, 50};   neoAdd(c1); // Blue
    int c2[3] = {0, 50, 0};   neoAdd(c2); // Green
    int c3[3] = {50, 0, 0};   neoAdd(c3); // Red
    int c4[3] = {50, 0, 50};  neoAdd(c4); // Purple
    int c5[3] = {50, 50, 0};  neoAdd(c5); // Yellow
    int c6[3] = {0, 50, 50};  neoAdd(c6); // Cyan
    int c7[3] = {50, 50, 50}; neoAdd(c7); // White
    int c8[3] = {0, 0, 0};    neoAdd(c8); // Off
}

/**
 * LOGIC: setPoliceSiren
 * [RULE 2 LOG] Wiping buffer to allow manual high-speed Red/Blue toggle.
 */
void setPoliceSiren() {
    Serial.println("[LED-STATE] Upload detected. Invoking neoClearAll() for Police Siren.");
    neoClearAll(); 
}

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
        setPoliceSiren(); 
        
        if(!filename.startsWith("/")) filename = "/" + filename;
        
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash > 0) {
            String path = filename.substring(0, lastSlash);
            if (!SD_MMC.exists(path)) {
                Serial.printf("[FS-LOG] Creating path: %s\n", path.c_str());
                SD_MMC.mkdir(path);
            }
        }
        
        Serial.printf("[FS-LOG] Writing: %s\n", filename.c_str());
        request->_tempFile = SD_MMC.open(filename, FILE_WRITE);
    }
    
    if (request->_tempFile) {
        request->_tempFile.write(data, len);
        
        // --- POLICE SIREN TOGGLE ---
        flip = !flip;
        if(flip) neoColor(50, 0, 0); // RED
        else neoColor(0, 0, 50);     // BLUE
    }
    
    if (final) {
        // --- END OF FILE UPLOAD ---
        Serial.printf("[UPLOAD-EVENT] Finalized: %s\n", filename.c_str());
        request->_tempFile.close();
        isUploading = false; 
        setNormalShuffle(); 
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); 
    Serial.println("\n[BOOT-LOG] Brainiac Ball Controller Online.");

    pinMode(Button, INPUT_PULLUP);
    neoPin(RGB_PIN);
    setNormalShuffle(); // Initial Sequence Load

    Serial.print("[HARDWARE-LOG] Gyroscope Initialization...");
    gyro.setup();
    Serial.println(" SUCCESS");

    int clk = 39, cmd = 38, d0 = 40;
    SD_MMC.setPins(clk, cmd, d0);
    Serial.print("[HARDWARE-LOG] SD_MMC Mount...");
    if(!SD_MMC.begin("/sdcard", true)) {
        Serial.println(" FAILED!");
    } else {
        Serial.println(" SUCCESS");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    dnsServer.start(53, "*", WiFi.softAPIP());
    MDNS.begin("ball");

    WebServer.serveStatic("/", SD_MMC, "/").setDefaultFile("login.html");

    WebServer.on("/upload", HTTP_DELETE, [](AsyncWebServerRequest *request) {
        if(request->hasHeader("X-File-Path")) {
            String path = request->getHeader("X-File-Path")->value();
            if(!path.startsWith("/")) path = "/" + path;
            if (SD_MMC.exists(path)) {
                SD_MMC.remove(path);
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

    if (!isUploading) {
        neoAuto(1000); 
    }

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