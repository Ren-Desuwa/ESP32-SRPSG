#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <Arduino.h>

struct GameSession {
    String patientID;
    String gameID;
    int sets;
    int reps;
    int score;
    String configSnapshot;
    bool isActive;
};

class SessionManager {
public:
    GameSession current;

    void start(String pid, String gid, String config) {
        current = {pid, gid, 0, 0, 0, config, true};
        Serial.printf("[SESSION] Started: %s playing %s\n", pid.c_str(), gid.c_str());
    }

    void update(int sets, int reps, int score) {
        if (!current.isActive) return;
        current.sets = sets;
        current.reps = reps;
        current.score = score;
    }

    String toCSV() {
        // Format: Timestamp(Placeholder),PID,GID,Sets,Reps,Score,Config
        // Note: Timestamp is added by DBManager or JS
        return current.patientID + "," + current.gameID + "," + 
               String(current.sets) + "," + String(current.reps) + "," + 
               String(current.score) + "," + current.configSnapshot;
    }

    void stop() {
        current.isActive = false;
        Serial.println("[SESSION] Stopped.");
    }
};

extern SessionManager Session;
SessionManager Session;

#endif