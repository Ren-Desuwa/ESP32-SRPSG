// /* [2026-01-30 - WROOM 38-PIN SD TEST] */
// /**
//  * Hardware: ESP32-WROOM-32 (38 Pin DevKit)
//  * Interface: SPI (Standard)
//  * Wiring: CS=5, MOSI=23, MISO=19, SCK=18
//  */

// #include <Arduino.h>
// #include <WiFi.h>
// #include <ESPAsyncWebServer.h>
// #include <FS.h>
// #include <SD.h>
// #include <SPI.h>

// // --- PIN DEFINITIONS (Standard VSPI) ---
// #define SD_CS   5
// #define SD_MOSI 23
// #define SD_MISO 19
// #define SD_SCK  18

// // --- LOGGING HELPER ---
// #define LOG(msg, ...) Serial.printf("[SD-SYS] " msg "\n", ##__VA_ARGS__)

// void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
//     LOG("Listing directory: %s", dirname);
//     File root = fs.open(dirname);
//     if(!root){
//         LOG("FAILED to open directory");
//         return;
//     }
//     if(!root.isDirectory()){
//         LOG("Not a directory");
//         return;
//     }

//     File file = root.openNextFile();
//     while(file){
//         if(file.isDirectory()){
//             Serial.print("  DIR : ");
//             Serial.println(file.name());
//             if(levels){
//                 listDir(fs, file.name(), levels -1);
//             }
//         } else {
//             Serial.print("  FILE: ");
//             Serial.print(file.name());
//             Serial.print("  SIZE: ");
//             Serial.println(file.size());
//         }
//         file = root.openNextFile();
//     }
// }

// void setup() {
//     Serial.begin(115200);
//     delay(2000); 

//     Serial.println("\n===========================================");
//     Serial.println("[BOOT] ESP32 WROOM SD Card Manager");
//     Serial.println("===========================================");

//     // 1. MANUALLY CONFIGURE SPI
//     // This ensures we are using the exact pins we think we are.
//     LOG("Initializing SPI Bus (SCK=%d, MISO=%d, MOSI=%d, CS=%d)...", SD_SCK, SD_MISO, SD_MOSI, SD_CS);
//     SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    
// }

// void loop() {
//     // Nothing to do
//     // 2. MOUNT SD CARD
//     // We pass the SPI instance and the CS pin.
//     if (!SD.begin(SD_CS, SPI)) {
//         LOG("CRITICAL: Card Mount Failed!");
//         LOG("Troubleshooting:");
//         LOG("1. Check wiring (MISO/MOSI often get swapped).");
//         LOG("2. Ensure SD card is FAT32 formatted.");
//         LOG("3. Check CS pin is GPIO 5.");
//         return;
//     }

//     // 3. CARD TYPE & SIZE
//     uint8_t cardType = SD.cardType();
//     if (cardType == CARD_NONE) {
//         LOG("No SD card attached (Wiring issue?)");
//         return;
//     }

//     String typeStr = "UNKNOWN";
//     if (cardType == CARD_MMC) typeStr = "MMC";
//     else if (cardType == CARD_SD) typeStr = "SDSC";
//     else if (cardType == CARD_SDHC) typeStr = "SDHC";

//     uint64_t cardSize = SD.cardSize() / (1024 * 1024);
//     LOG("SD Mounted Successfully: Type=%s, Size=%llu MB", typeStr.c_str(), cardSize);

//     // 4. TEST WRITE
//     LOG("Performing Write Test...");
//     File file = SD.open("/wroom_test.txt", FILE_WRITE);
//     if(file){
//         file.println("This is a test from the ESP32 WROOM via SPI.");
//         file.close();
//         LOG("Write OK.");
//     } else {
//         LOG("Write FAILED. Check permissions/lock switch.");
//     }

//     // 5. READ BACK
//     listDir(SD, "/", 0);
//     LOG("System Ready.");

//     delay(1000);
// }