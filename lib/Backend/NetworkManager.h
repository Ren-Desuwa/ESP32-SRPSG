#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <functional> 
#include "DBManager.h"

class NetworkManager {
public:
    AsyncWebServer* server;
    DNSServer* dns;
    AsyncWebSocket* ws;

    NetworkManager(AsyncWebServer* s, DNSServer* d, AsyncWebSocket* w) {
        server = s;
        dns = d;
        ws = w;
    }

    void init(const char* ssid) {
        // 1. Start Access Point
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ssid);
        Serial.print("[NET] AP Started: ");
        Serial.println(WiFi.softAPIP());

        // 2. Start mDNS (The "Name Tag" Service)
        // This allows you to visit: http://akbay.local
        if (MDNS.begin("akbay")) { 
            Serial.println("[NET] mDNS Responder Started: http://akbay.local");
        } else {
            Serial.println("[NET] Error setting up mDNS!");
        }

        // 3. Start DNS Server (Captive Portal)
        // The "*" redirects ALL traffic (google.com, apple.com) to us.
        // This effectively allows http://akbay.system to work too!
        dns->start(53, "*", WiFi.softAPIP());
    }

    // Accepts a callback to toggle the "Police Siren" LED effect
    void setupRoutes(std::function<void(bool)> onBusyState = nullptr) {
        
        // --- 1. STATIC FILES ---
        server->serveStatic("/", SD, "/").setDefaultFile("akbay.html");

        // --- 2. DATABASE API (For Website JS) ---
        server->on("/db/read", HTTP_GET, [](AsyncWebServerRequest *req){ 
            DB.handleRead(req); 
        });

        server->on("/db/write", HTTP_POST, 
            [](AsyncWebServerRequest *req){}, 
            NULL,
            [onBusyState](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
                if (index == 0 && onBusyState) onBusyState(true);
                DB.handleWrite(req, data, len, index, total);
                if (index + len == total && onBusyState) onBusyState(false);
            }
        );

        // --- 3. LEGACY DEPLOYMENT API (For Python Script) ---
        
        // DELETE Handler
        server->on("/upload", HTTP_DELETE, [](AsyncWebServerRequest *req) {
            if(req->hasHeader("X-File-Path")) {
                String path = req->getHeader("X-File-Path")->value();
                if(!path.startsWith("/")) path = "/" + path;
                
                if (SD.exists(path)) {
                    SD.remove(path);
                    Serial.printf("[DEPLOY] Deleted: %s\n", path.c_str());
                    req->send(200, "text/plain", "Deleted");
                } else {
                    req->send(404, "text/plain", "File Not Found");
                }
            } else {
                req->send(400, "text/plain", "Missing X-File-Path");
            }
        });

        // UPLOAD Handler
        server->on("/upload", HTTP_POST, 
            [](AsyncWebServerRequest *req){ req->send(200); }, // Send 200 OK on finish
            [onBusyState](AsyncWebServerRequest *req, String filename, size_t index, uint8_t *data, size_t len, bool final) {
                // 1. Start: Trigger LEDs and Open File
                if (index == 0) {
                    if(onBusyState) onBusyState(true);
                    
                    // Use Header path if available (Python Script), else filename
                    if(req->hasHeader("X-File-Path")) {
                        filename = req->getHeader("X-File-Path")->value();
                    }
                    if(!filename.startsWith("/")) filename = "/" + filename;

                    Serial.printf("[DEPLOY] Upload Start: %s\n", filename.c_str());

                    // Ensure directory exists
                    int lastSlash = filename.lastIndexOf('/');
                    if (lastSlash > 0) {
                        String folder = filename.substring(0, lastSlash);
                        if (!SD.exists(folder)) SD.mkdir(folder);
                    }
                    
                    req->_tempFile = SD.open(filename, FILE_WRITE);
                }

                // 2. Write Chunk
                if (req->_tempFile) {
                    req->_tempFile.write(data, len);
                }

                // 3. End: Close File and Stop LEDs
                if (final) {
                    if (req->_tempFile) req->_tempFile.close();
                    Serial.println("[DEPLOY] Upload Complete.");
                    if(onBusyState) onBusyState(false);
                }
            }
        );

        // --- 4. 404 & CAPTIVE PORTAL ---
        // Change this line in NetworkManager.h inside setupRoutes()
        server->onNotFound([this](AsyncWebServerRequest *req){ 
            // Redirect to the .local address instead of the IP
            req->redirect("http://akbay.local/"); 
        });

        server->addHandler(ws);
        server->begin();
        Serial.println("[NET] WebServer & Deployment API Online.");
    }
};

#endif