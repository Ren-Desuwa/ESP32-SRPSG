/* [2026-01-29 12:00 pm - batch 2.6.0] */
#ifndef NEO_H
#define NEO_H

#include <Arduino.h>

// SENIOR DEV NOTE: 
// 256 is the optimal size because it matches the rollover of an 8-bit integer.
// We change storage to uint8_t to save 50-75% RAM.
#define MAX_COLORS 256 

extern long lastms;
extern int Pin;

// Storage is now uint8_t (0-255), but we can still accept 'int' as input
extern uint8_t colors[MAX_COLORS][3]; 
extern int slot; 

// API
void neoOff();
void neoPin(int pin);
void neoColor(int red, int green, int blue);
void neoShuffle();
void neoNext();
void neoPrev();

/* [SYSTEM LOG] Adding neoClearAll to header for state management */
void neoClearAll();

// Updated to allow a custom limit
// If loopLimit is -1, it loops through ALL available colors.
// If loopLimit is 5, it loops 0->1->2->3->4->5->0
void neoAuto(int ms, int loopLimit = -1);

int neoAdd(int inputColors[3], int customslot = -1); 
int neoRemove(int targetSlot);

#endif //NEO_H