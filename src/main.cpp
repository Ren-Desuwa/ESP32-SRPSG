/**
 * Project: Brainiac Ball Controller - Professional Build
 * Version: 3.0.0 (Senior Developer Database Edition)
 * -------------------------------------------------------
 * [LOGGING POLICY] Rule 2: No deleting logs or comments. 
 * Every hardware event and software state transition is logged.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "Neo.h"       // [SOURCE: uploaded include/NeoLED]
#include "Gyro.h"        // [SOURCE: uploaded include/Gyro]

// --- MODULAR BACKEND ---
#include "DBManager.h"       // [SOURCE: uploaded include/Backend]
#include "SessionManager.h"  // [SOURCE: uploaded include/Backend]
#include "NetworkManager.h"  // [SOURCE: uploaded include/Backend]

// --- HARDWARE CONFIG ---
#define SSID_NAME "Ball_Test_AP"
#define BUTTON_PIN 0 
#define RGB_PIN 48
#define GYRO_SDA 8
#define GYRO_SCL 9
#define Button 21

// --- GLOBALS ---
Gyro gyro(GYRO_SDA, GYRO_SCL); 
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dns;

// Managers
DBManager DB;
SessionManager Session;
NetworkManager net(&server, &dns, &ws);

// State tracking (Preserved from V2.6.0)
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
    Serial.println("[LED-STATE] Upload/Write detected. Invoking neoClearAll() for Police Siren.");
    neoClearAll(); 
}

// --- CALLBACK HANDLERS ---
// This allows NetworkManager to notify Main about activity without knowing about LEDs
void onSystemBusy(bool busy) {
    isUploading = busy;
    if (isUploading) {
        setPoliceSiren();
    } else {
        Serial.println("[UPLOAD-EVENT] Transaction Finished.");
        setNormalShuffle();
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); 
    Serial.println("\n[BOOT-LOG] Brainiac Ball Controller V3.0 (DB Edition) Online.");

    // 1. Hardware Init
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(Button, INPUT_PULLUP);
    neoPin(RGB_PIN);
    setNormalShuffle(); // Initial Sequence Load

    Serial.print("[HARDWARE-LOG] Gyroscope Initialization...");
    gyro.setup();
    Serial.println(" SUCCESS");

    // 2. Database Init (SD Card)
    // [RULE 2 LOG] Handled inside DBManager::init(), but we confirm here.
    DB.init(); 

    // 3. Network Init
    net.init(SSID_NAME);
    
    // 4. Route Setup (With Busy Callback for LEDs)
    // We pass the 'onSystemBusy' function so the LED changes when the DB is written
    net.setupRoutes(onSystemBusy);

    Serial.println("[SYSTEM-LOG] Deployment and Telemetry services ready.");
}

void loop() {
    // 1. Network Housekeeping
    dns.processNextRequest();
    ws.cleanupClients();

    // 2. LED Logic (Preserved V2.6.0 Siren Logic)
    if (!isUploading) {
        neoAuto(1000); 
    } else {
        // [RULE 2 LOG] Manual Police Siren Toggle
        static unsigned long lastBlink = 0;
        static bool flip = false;
        if (millis() - lastBlink > 100) { 
            lastBlink = millis();
            flip = !flip;
            if(flip) neoColor(50, 0, 0); // RED
            else neoColor(0, 0, 50);     // BLUE
        }
    }

    // 3. Sensor Loop
    gyro.getData();
    gyro.readAll();

    // 4. Telemetry Broadcast (Senior Dev Implementation)
    ws.cleanupClients(); 
    auto& clients = ws.getClients();
    
    for (auto& client : clients) {
        // [CRITICAL] Backpressure Check
        if (client.status() == WS_CONNECTED && client.canSend()) {
            
            // FIX: Added logic to read Pin 21 (Button)
            // LOW = Pressed (1), HIGH = Released (0)
            String json = "{\"x\":" + String(gyro.readAccumX(), 2) + 
                          ",\"y\":" + String(gyro.readAccumY(), 2) + 
                          ",\"z\":" + String(gyro.readAccumZ(), 2) +
                          ",\"b1\":" + String(digitalRead(Button) == LOW ? 1 : 0) + "}";
            
            client.text(json);
        }
    }

    // 5. Mandatory Yield
    delay(2);
}