/* [2026-01-29 12:00 pm - batch 2.6.0] */
#include "Neo.h"

long lastms = 0;
int Pin = -1;
uint8_t colors[MAX_COLORS][3]; // Optimized memory usage
int slot = 0; 
static int playIndex = 0; 

// --- Internal Helper ---
void neoApply() {
    if (slot == 0) { 
        neoOff(); 
        return; 
    }
    // Safety checks
    if (playIndex >= slot) playIndex = 0;
    
    // Cast uint8_t back to int for the hardware function
    neopixelWrite(Pin, (int)colors[playIndex][0], (int)colors[playIndex][1], (int)colors[playIndex][2]);
}

// --- API ---

void neoOff() {
    neopixelWrite(Pin, 0, 0, 0);
}

void neoPin(int pin) {
    Pin = pin;
    pinMode(Pin, OUTPUT);
}

void neoColor(int red, int green, int blue) {
    neopixelWrite(Pin, red, green, blue);
}

void neoShuffle() {
    // Standard Fisher-Yates Shuffle
    for (int i = slot - 1; i > 0; i--) {
        int j = random(0, i + 1);
        for(int k=0; k<3; k++) {
            uint8_t temp = colors[i][k]; // Note the type change here
            colors[i][k] = colors[j][k];
            colors[j][k] = temp;
        }
    }
    playIndex = 0;
}

void neoNext() {
    if (slot == 0) return;
    playIndex++;
    if (playIndex >= slot) playIndex = 0;
    neoApply();
}

void neoPrev() {
    if (slot == 0) return;
    playIndex--;
    if (playIndex < 0) playIndex = slot - 1;
    neoApply();
}

// New Logic Implementation
void neoAuto(int ms, int loopLimit) {
    if (millis() - lastms >= ms) {
        lastms = millis();

        // If user didn't set a limit (-1), default to the total available slots
        int effectiveLimit = (loopLimit == -1) ? slot : loopLimit;

        // Ensure we don't try to loop past valid data even if the user asks for it
        if (effectiveLimit > slot) effectiveLimit = slot;

        playIndex++;

        // The specific logic requested: 
        // "after the 5th slot it resets to 0" (means index > 5 resets)
        if (playIndex >= effectiveLimit) {
            playIndex = 0;
        }

        neoApply();
    }
}

int neoAdd(int inputColors[3], int customslot) {
    int target = (customslot == -1) ? slot : customslot;

    // Safety check against MAX_COLORS
    if (target >= MAX_COLORS) return -1; 
    
    // Implicit cast from int (input) to uint8_t (storage)
    // This automatically handles the 0-255 constraint
    colors[target][0] = inputColors[0];
    colors[target][1] = inputColors[1];
    colors[target][2] = inputColors[2];

    if (target == slot) {
        slot++;
    }
    return target;
}

int neoRemove(int targetSlot) {
    if (targetSlot < 0 || targetSlot >= slot) return -1;

    for (int i = targetSlot; i < slot - 1; i++) {
        colors[i][0] = colors[i+1][0];
        colors[i][1] = colors[i+1][1];
        colors[i][2] = colors[i+1][2];
    }
    
    // Zero out last slot
    colors[slot-1][0] = 0; colors[slot-1][1] = 0; colors[slot-1][2] = 0;

    slot--;
    if (playIndex >= slot && slot > 0) playIndex = slot - 1;
    
    return slot;
}

/* [SYSTEM LOG] Implementing neoClearAll to wipe the color buffer */
void neoClearAll() {
    Serial.println("[LED-LIB] neoClearAll() called. Resetting all stored color slots.");
    // This resets the internal counter/index of the neoAdd sequence
    slot = 0; 
    playIndex = 0;
}