/* [2026-01-30] EspNowHub.cpp - Protocol Fix Edition */
#include "EspNowHub.h"

EspNowHub* EspNowHub::_instance = nullptr;

EspNowHub::EspNowHub() { _instance = this; }

void EspNowHub::begin(String myName) {
    _myName = myName;
    WiFi.mode(WIFI_AP_STA); 
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("[HUB] ESP-NOW Init Failed!");
        return;
    }

    esp_now_register_recv_cb(_onDataRecv);
    esp_now_register_send_cb(_onDataSent);

    // Broadcast Peer
    uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    _addPeer(broadcastMac);
    broadcast("READY");
    Serial.printf("[HUB] Started as '%s'\n", _myName.c_str());
}

bool EspNowHub::broadcastData(const void* data, size_t len) {
    return sendData("BROADCAST", data, len);
}

bool EspNowHub::sendData(String targetName, const void* data, size_t len) {
    if (len > MAX_DATA_LEN) return false;
    Packet pkt;
    pkt.type = TYPE_DATA;
    strncpy(pkt.senderName, _myName.c_str(), MAX_NAME_LEN);
    memcpy(pkt.payload, data, len);

    uint8_t destMac[6];
    if (targetName == "BROADCAST") memset(destMac, 0xFF, 6);
    else {
        int idx = _findPeerByName(targetName);
        if (idx == -1) return false;
        memcpy(destMac, _peers[idx].mac, 6);
    }
    
    // [FIX] Send ONLY header + actual data length (14 bytes), not the full 200 bytes padding
    size_t packetSize = sizeof(pkt.type) + sizeof(pkt.senderName) + len;
    
    return (esp_now_send(destMac, (uint8_t *) &pkt, packetSize) == ESP_OK);
}

bool EspNowHub::broadcast(String message) {
    return broadcastData(message.c_str(), message.length() + 1);
}

bool EspNowHub::send(String targetName, String message) {
    return sendData(targetName, message.c_str(), message.length() + 1);
}

void EspNowHub::setOnReceive(OnDataReceivedCallback cb) {
    _userCallback = cb;
}

void EspNowHub::_onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == 0) return;
    if (_instance) _instance->_handlePacket(mac, (Packet*)incomingData, len);
}

void EspNowHub::_handlePacket(const uint8_t* mac, Packet* packet, size_t len) {
    if (!_peerExists(mac)) {
        PeerEntry newPeer;
        memcpy(newPeer.mac, mac, 6);
        strncpy(newPeer.name, packet->senderName, MAX_NAME_LEN);
        _peers.push_back(newPeer);
        _addPeer(mac);
        Serial.printf("[HUB] New Peer: %s\n", newPeer.name);
    }

    if (_userCallback) {
        // Calculate actual payload length safely
        size_t headerSize = sizeof(Packet::type) + sizeof(Packet::senderName);
        if (len > headerSize) {
            _userCallback(String(packet->senderName), packet->payload, len - headerSize);
        }
    }
}

void EspNowHub::_onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {}

bool EspNowHub::_addPeer(const uint8_t* mac) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    return (esp_now_add_peer(&peerInfo) == ESP_OK);
}

bool EspNowHub::_peerExists(const uint8_t* mac) {
    for (const auto& peer : _peers) if (memcmp(peer.mac, mac, 6) == 0) return true;
    return false;
}

int EspNowHub::_findPeerByName(String name) {
    for (int i = 0; i < _peers.size(); i++) if (String(_peers[i].name) == name) return i;
    return -1;
}

void EspNowHub::listPeers() {}