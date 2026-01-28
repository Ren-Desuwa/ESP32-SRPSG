#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <FS.h>
#include <SD_MMC.h>
#include <ESPAsyncWebServer.h>

class DBManager {
public:
    bool isReady = false;

    void init() {
        int clk = 39, cmd = 38, d0 = 40;
        SD_MMC.setPins(clk, cmd, d0);
        if (!SD_MMC.begin("/sdcard", true)) {
            Serial.println("[DB] SD Mount Failed!");
            return;
        }
        
        isReady = true;
        Serial.println("[DB] SD Mounted Successfully.");
        
        // Ensure standard folders exist
        createDir("/database");
        createDir("/database/patients");
    }

    void createDir(const char* path) {
        if (!SD_MMC.exists(path)) {
            SD_MMC.mkdir(path);
            Serial.printf("[DB] Created Dir: %s\n", path);
        }
    }

    // The Core "Write" Logic
    void handleWrite(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (!isReady) { request->send(500, "text/plain", "SD Not Ready"); return; }

        if (!request->hasParam("file")) { request->send(400, "text/plain", "Missing 'file'"); return; }
        
        String filename = "/database/" + request->getParam("file")->value();
        // Remove double slashes if any
        filename.replace("//", "/");

        String modeStr = "append";
        if (request->hasParam("mode")) modeStr = request->getParam("mode")->value();

        if (index == 0) {
            // Auto-create directory if missing
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash > 0) {
                String folder = filename.substring(0, lastSlash);
                if (!SD_MMC.exists(folder)) SD_MMC.mkdir(folder);
            }

            const char* mode = (modeStr == "write") ? FILE_WRITE : FILE_APPEND;
            request->_tempFile = SD_MMC.open(filename, mode);
            Serial.printf("[DB] Writing %s (%s)\n", filename.c_str(), modeStr.c_str());
        }

        if (request->_tempFile) {
            request->_tempFile.write(data, len);
        }

        if (index + len == total) {
            request->_tempFile.close();
            request->send(200, "text/plain", "Saved");
            // NOTE: We will hook into WebSocket here later for "Town Crier" updates
        }
    }
    
    // The Core "Read" Logic
    void handleRead(AsyncWebServerRequest *request) {
        if (!isReady) { request->send(500, "text/plain", "SD Not Ready"); return; }
        
        if (request->hasParam("file")) {
            String fileName = "/database/" + request->getParam("file")->value();
            fileName.replace("//", "/");
            
            if (SD_MMC.exists(fileName)) {
                request->send(SD_MMC, fileName, "text/plain");
            } else {
                request->send(404, "text/plain", "File not found");
            }
        } else {
            request->send(400, "text/plain", "Missing file param");
        }
    }
};

extern DBManager DB;
DBManager DB;

#endif