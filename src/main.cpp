/**
 * Project: Brainiac Ball Controller - Hub Unit (WROOM-32 Edition)
 * Role: CENTRAL SERVER & BRIDGE
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <SD.h> 
#include <SPI.h>
#include "EspNowHub.h"      
#include "Gyro.h"

// --- MODULAR BACKEND ---
#include "DBManager.h"       
#include "SessionManager.h"  
#include "NetworkManager.h"  

// --- SHARED PROTOCOL ---
struct MotionData {
    float x;      // 4 bytes (Gyro Angle)
    float y;      // 4 bytes
    float z;      // 4 bytes
    
    int16_t lax;  // 2 bytes (Linear Accel X - Extension)
    int16_t lay;  // 2 bytes (Linear Accel Y)
    int16_t laz;  // 2 bytes (Linear Accel Z)
    
    int16_t gx;   // 2 bytes (Gravity X - Scaled Float)
    int16_t gy;   // 2 bytes (Gravity Y)
    int16_t gz;   // 2 bytes (Gravity Z)
    
    uint8_t btn1; // 1 byte
    uint8_t btn2; // 1 byte
} __attribute__((packed)); // Total: 26 bytes

// --- CONFIGURATION ---
#define AP_SSID  "Akbay"
#define GYRO_SDA 21 
#define GYRO_SCL 22

#define BUTTON_1 32
#define BUTTON_2 33

// --- GLOBALS ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dns;
EspNowHub Hub;
Gyro gyro(GYRO_SDA, GYRO_SCL); 

// Managers
DBManager DB;
SessionManager Session;
NetworkManager net(&server, &dns, &ws);

// --- STATE TRACKING ---
// [RESTORED] Mailbox to hold the REAL glove data
volatile MotionData mailbox;
// Timestamp to track if the glove is alive
volatile unsigned long lastGloveTime = 0;

// --- CALLBACK: ESP-NOW PACKET RECEIVED ---
void onHubReceive(const String& sender, const uint8_t* payload, size_t len) {
    if (len != sizeof(MotionData)) return;
    
    // [UPDATED] Save the real data AND update the timestamp
    memcpy((void*)&mailbox, payload, sizeof(MotionData));
    lastGloveTime = millis();
}

// --- SYSTEM BUSY CALLBACK ---
void onSystemBusy(bool busy) {
    if (busy) {
        Serial.println("[SYSTEM-STATE] Status: BUSY (SD Write)");
    } else {
        Serial.println("[SYSTEM-STATE] Status: IDLE");
    }
}

// --- WEBSOCKET EVENT HANDLER ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client #%u connected\n", client->id());
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); 

    pinMode(BUTTON_1, INPUT_PULLUP);
    pinMode(BUTTON_2, INPUT_PULLUP);
    
    Serial.println("\n[BOOT] BRAINIAC WROOM-32 HUB ONLINE");

    ws.onEvent(onWsEvent);

    // 1. GYRO INIT (WROOM Pins)
    gyro.setup();

    // 2. DATABASE INIT (WROOM SPI Pins)
    DB.init(); 

    // 3. NETWORK INIT
    net.init(AP_SSID);
    net.setupRoutes(onSystemBusy); 
    
    // 4. ESP-NOW INIT
    Hub.begin("MainHub"); 
    Hub.setOnReceive(onHubReceive);

    Serial.println("[SYSTEM] Ready. AP: Brainiac_Host");
}

void loop() {
    dns.processNextRequest();
    ws.cleanupClients(); 

    gyro.getData(); 
    gyro.readAll(); 

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 50) {
        lastSend = millis();

        // 1. GET NEW DATA
        int16_t lax, lay, laz;
        float gx, gy, gz;
        
        gyro.getLinearAccel(lax, lay, laz);
        gyro.getGravity(gx, gy, gz);

        // 2. ARM DATA
        String armJson = "{";
        armJson += "\"device\":\"Arm\","; 
        armJson += "\"x\":" + String(gyro.readX(), 2) + ",";
        armJson += "\"y\":" + String(gyro.readY(), 2) + ",";
        armJson += "\"z\":" + String(gyro.readZ(), 2) + ",";
        
        // Linear Accel (Movement)
        armJson += "\"lax\":" + String(lax) + ",";
        armJson += "\"lay\":" + String(lay) + ",";
        armJson += "\"laz\":" + String(laz) + ",";
        
        // Gravity (Orientation) - Scaled to Int
        armJson += "\"gx\":" + String((int)(gx * 1000)) + ",";
        armJson += "\"gy\":" + String((int)(gy * 1000)) + ",";
        armJson += "\"gz\":" + String((int)(gz * 1000)) + ",";
        
        armJson += "\"b1\":" + String(digitalRead(BUTTON_1) == LOW ? 1 : 0) + ",";
        armJson += "\"b2\":" + String(digitalRead(BUTTON_2) == LOW ? 1 : 0);
        armJson += "}";
        
        // 3. GLOVE DATA
        bool isGloveConnected = (millis() - lastGloveTime) < 1500;
        String gloveJson = "{";
        gloveJson += "\"device\":\"Glove\","; 
        
        if (isGloveConnected) {
            gloveJson += "\"x\":" + String(mailbox.x, 2) + ",";
            gloveJson += "\"y\":" + String(mailbox.y, 2) + ",";
            gloveJson += "\"z\":" + String(mailbox.z, 2) + ",";
            
            gloveJson += "\"lax\":" + String(mailbox.lax) + ",";
            gloveJson += "\"lay\":" + String(mailbox.lay) + ",";
            gloveJson += "\"laz\":" + String(mailbox.laz) + ",";
            
            gloveJson += "\"gx\":" + String(mailbox.gx) + ",";
            gloveJson += "\"gy\":" + String(mailbox.gy) + ",";
            gloveJson += "\"gz\":" + String(mailbox.gz) + ",";
            
            gloveJson += "\"b1\":" + String(mailbox.btn1) + ",";
            gloveJson += "\"b2\":1"; 
        } else {
            gloveJson += "\"x\":0,\"y\":0,\"z\":0,";
            gloveJson += "\"lax\":0,\"lay\":0,\"laz\":0,";
            gloveJson += "\"gx\":0,\"gy\":0,\"gz\":0,";
            gloveJson += "\"b1\":0,\"b2\":0"; 
        }
        gloveJson += "}";

        if (ws.count() > 0) {
            ws.textAll("[" + armJson + "," + gloveJson + "]");
        }
    }
    delay(2); 
}