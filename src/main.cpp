/* [2026-01-30] Brainiac Main Hub (Pure Server Edition) */
/**
 * Project: Brainiac Ball Controller - Hub Unit
 * Role: CENTRAL SERVER & BRIDGE
 * -------------------------------------------------------
 * [SYSTEM ARCHITECTURE]
 * 1. WiFi AP: Creates "Brainiac_Host" for the Laptop/Phone to connect.
 * 2. Web Server: Serves HTML/JS/CSS from the SD Card (via DBManager).
 * 3. ESP-NOW: Listens for packets from "Glove" and "Arm".
 * 4. WebSocket: Forwards that data to the browser in real-time.
 * -------------------------------------------------------
 * [LOGGING POLICY] Rule 2: VERBOSE. Log every packet, error, and state change.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "EspNowHub.h"      // [SOURCE: Local Library with Binary Fix]
#include "Neo.h"            // [SOURCE: Local Library]

// --- MODULAR BACKEND ---
#include "DBManager.h"       
#include "SessionManager.h"  
#include "NetworkManager.h"  

// --- SHARED PROTOCOL (CRITICAL) ---
// This structure MUST match the Senders (Glove/Arm) byte-for-byte.
// Total Size: 4 + 4 + 4 + 1 + 1 = 14 Bytes.
struct MotionData {
    float x;      // 4 bytes (Gyro X)
    float y;      // 4 bytes (Gyro Y)
    float z;      // 4 bytes (Gyro Z)
    uint8_t btn1; // 1 byte  (Primary Click)
    uint8_t btn2; // 1 byte  (Secondary Click - Arm only)
} __attribute__((packed));

// --- CONFIGURATION ---
#define AP_SSID  "Brainiac_Host"
#define RGB_PIN  48

// --- GLOBALS ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dns;
EspNowHub Hub;

// Managers
DBManager DB;
SessionManager Session;
NetworkManager net(&server, &dns, &ws);

// --- LED HELPER ---
void setStatus(int r, int g, int b) {
    neoClearAll();
    int c[3] = {r, g, b};
    neoAdd(c);
}
// --- MAILBOX (THREAD-SAFE STORAGE) ---
// We store the latest packet here instead of sending it immediately
volatile MotionData mailbox; 
volatile bool mailReady = false;
// --- CALLBACK: ESP-NOW PACKET RECEIVED ---
/**
 * Triggered when a Sender (Glove or Arm) broadcasts data.
 * We must unpack the binary struct and convert it to JSON for the Web Client.
 */
void onHubReceive(const String& sender, const uint8_t* payload, size_t len) {
    // 1. Integrity Check
    if (len != sizeof(MotionData)) return;

    // 2. Save to Mailbox (Overwrite old data - we only want the newest)
    memcpy((void*)&mailbox, payload, sizeof(MotionData));
    
    // 3. Raise the flag so loop() knows there is mail
    mailReady = true;
}
// void onHubReceive(const String& sender, const uint8_t* payload, size_t len) {
//     // [RULE 2] LOGGING IS KING: Log the incoming packet details
//     // Serial.printf("[HUB-RX] Data from '%s' | Size: %d bytes\n", sender.c_str(), len);
//     static unsigned long lastBroadcast = 0;
//     if (millis() - lastBroadcast < 30) {
//         return; // SKIP this packet. Too fast!
//     }
//     lastBroadcast = millis();

//     // 1. SECURITY / INTEGRITY CHECK
//     if (len != sizeof(MotionData)) {
//         Serial.printf("[HUB-ERROR] PACKET SIZE MISMATCH! Sender: %s | Expected: %d | Got: %d\n", 
//             sender.c_str(), sizeof(MotionData), len);
//         return; // Drop bad packet to prevent crash
//     }

//     // 2. UNPACK BINARY DATA
//     // We cast the raw byte array to our struct pointer
//     const MotionData* data = (const MotionData*) payload;

//     // [RULE 2] DEBUG LOGGING (Throttle this if it floods the console too much)
//     // Serial.printf("[DATA] %s -> X:%.2f Y:%.2f Z:%.2f B1:%d B2:%d\n", 
//     //     sender.c_str(), data->x, data->y, data->z, data->btn1, data->btn2);

//     // 3. CONVERT TO JSON FOR BROWSER
//     // The website's 'mouse.js' expects specific keys: 'device', 'x', 'y', 'z', 'b1'.
//     String json = "{";
//     json += "\"device\":\"" + sender + "\","; 
//     json += "\"x\":" + String(data->x, 2) + ",";
//     json += "\"y\":" + String(data->y, 2) + ",";
//     json += "\"z\":" + String(data->z, 2) + ",";
//     json += "\"b1\":" + String(data->btn1) + ","; // Primary Click
//     json += "\"b2\":" + String(data->btn2);       // Secondary Click (Context/Right)
//     json += "}";

//     // Serial.println(json);

//     // 4. FORWARD TO WEBSOCKET CLIENTS
//     if (ws.count() > 0) {
//         ws.textAll(json);
//     } else {
//         // Log warning if we are receiving data but no one is listening
//         static unsigned long lastWarn = 0;
//         if (millis() - lastWarn > 2000) {
//             lastWarn = millis();
//             Serial.println("[HUB-WARN] Receiving ESP-NOW data, but NO Browser Clients connected!");
//         }
//     }
// }

// --- SYSTEM BUSY CALLBACK ---
// This allows the NetworkManager to signal file operations (like SD Card uploads)
void onSystemBusy(bool busy) {
    if (busy) {
        Serial.println("[SYSTEM-STATE] Status: BUSY (File Operation in Progress)");
        setStatus(50, 0, 0); // RED = Do not power off
    } else {
        Serial.println("[SYSTEM-STATE] Status: IDLE (Ready for Requests)");
        setStatus(0, 50, 0); // GREEN = All good
    }
}

// --- WEBSOCKET EVENT HANDLER ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS-LOG] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS-LOG] Client #%u disconnected!\n", client->id());
            break;
        case WS_EVT_ERROR:
            Serial.printf("[WS-LOG] Client #%u error: %s\n", client->id(), (char*)arg);
            break;
        case WS_EVT_PONG:
            Serial.printf("[WS-LOG] Client #%u pong\n", client->id());
            break;
    }
}



void setup() {
    Serial.begin(115200);
    delay(2000); // Allow USB Serial to catch up
    
    // 1. HARDWARE INIT
    Serial.println("\n\n===============================================");
    Serial.println("[BOOT] BRAINIAC CONTROLLER HUB V3.3");
    Serial.println("[INFO] Role: Dedicated Server & Bridge");
    Serial.println("===============================================");
    
    neoPin(RGB_PIN);
    setStatus(0, 0, 50); // BLUE = Booting Phase
    Serial.println("[BOOT] LED System Initialized.");

    ws.onEvent(onWsEvent);

    // 2. DATABASE INIT
    // Checks SD Card, mounts filesystem, prepares storage
    Serial.print("[BOOT] Initializing Database Manager... ");
    DB.init(); // DBManager handles its own detailed logging
    Serial.println("[BOOT] Database Manager: OK");

    // 3. NETWORK INIT (AP MODE)
    Serial.printf("[BOOT] Starting Access Point: '%s'...\n", AP_SSID);
    net.init(AP_SSID); 
    net.setupRoutes(onSystemBusy); // Pass our LED callback
    
    IPAddress ip = WiFi.softAPIP();
    Serial.print("[NETWORK] AP Created Successfully. IP Address: ");
    Serial.println(ip);

    // 4. ESP-NOW INIT
    Serial.println("[BOOT] Initializing ESP-NOW Hub...");
    Hub.begin("MainHub"); // We identify ourselves as "MainHub"
    
    // Register the function that handles incoming data
    Hub.setOnReceive(onHubReceive);
    Serial.println("[ESP-NOW] Listener Active. Waiting for 'Glove' or 'Arm'...");

    // 5. FINAL STATE
    setStatus(0, 50, 0); // GREEN = Ready
    Serial.println("[SYSTEM] Startup Complete. System is READY.");
    Serial.println("[HELP] Connect to Wi-Fi 'Brainiac_Host' and go to http://192.168.4.1");
}

int i = 0;
void loop() {
    // 1. Network Housekeeping
    dns.processNextRequest();
    ws.cleanupClients(); 
    
    // 2. CHECK MAILBOX & SEND SAFELY
    if (mailReady) {
        i++;
        // Lower the flag immediately
        mailReady = false; 

        static unsigned long lastSend = 0;
        if (millis() - lastSend < 5) { 
            return; // SKIP! Drop this packet. We only want the latest *renderable* frame.
        }
        lastSend = millis();

        // Build JSON (Only do this ONCE per loop, not per client)
        // Note: You'll need to handle the "device" name logic if you have multiple devices. 
        // For now, we assume "Glove" or pass the sender name in the struct if needed.
        String json = "{";
        json += "\"x\":" + String(mailbox.x, 2) + ",";
        json += "\"y\":" + String(mailbox.y, 2) + ",";
        json += "\"z\":" + String(mailbox.z, 2) + ",";
        json += "\"b1\":" + String(mailbox.btn1) + ","; 
        json += "\"b2\":" + String(mailbox.btn2);
        json += "}";

        // THE CRITICAL FIX:
        auto& clients = ws.getClients();
        for (auto& client : clients) {
            // Ask: "Are you ready?"
            if (client.status() == WS_CONNECTED && client.canSend()) {
                client.text(json); // Only send if YES
            }
        }

        // Serial.println(i);
    }

    // 3. Keep the Heartbeat Logic
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 5000) {
        lastHeartbeat = millis();
        Serial.printf("[STATUS] Heartbeat | Clients: %u | Free Heap: %u bytes\n", 
            ws.count(), ESP.getFreeHeap());
    }

    // 4. Critical Delay
    delay(2); 
}