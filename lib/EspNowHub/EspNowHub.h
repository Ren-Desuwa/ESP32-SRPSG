/* [2026-01-30] EspNowHub.h - Binary Protocol Edition */
#ifndef ESP_NOW_HUB_H
#define ESP_NOW_HUB_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <vector>

#define MAX_PEERS 20
#define MAX_NAME_LEN 16
#define MAX_DATA_LEN 200 

enum PacketType { TYPE_HELLO = 0, TYPE_DATA = 1 };

struct Packet {
    uint8_t type;
    char senderName[MAX_NAME_LEN];
    uint8_t payload[MAX_DATA_LEN]; // Changed to uint8_t array for binary safety
} __attribute__((packed));

struct PeerEntry {
    char name[MAX_NAME_LEN];
    uint8_t mac[6];
};

// [CRITICAL CHANGE] This matches the signature in your main.cpp
typedef void (*OnDataReceivedCallback)(const String& sender, const uint8_t* payload, size_t len);

class EspNowHub {
public:
    EspNowHub();
    void begin(String myName);
    
    // Binary Sending (Preferred)
    bool sendData(String targetName, const void* data, size_t len);
    bool broadcastData(const void* data, size_t len);
    
    // String Sending (Legacy support)
    bool send(String targetName, String message);
    bool broadcast(String message);
    
    void setOnReceive(OnDataReceivedCallback cb);
    void listPeers();

private:
    String _myName;
    OnDataReceivedCallback _userCallback;
    std::vector<PeerEntry> _peers;

    static EspNowHub* _instance;
    static void _onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
    static void _onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

    void _handlePacket(const uint8_t* mac, Packet* packet, size_t len);
    bool _addPeer(const uint8_t* mac);
    int _findPeerByName(String name);
    bool _peerExists(const uint8_t* mac);
};

#endif