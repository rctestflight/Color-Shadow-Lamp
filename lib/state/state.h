// state.h
#ifndef STATE_H
#define STATE_H

#include <stdint.h>

// Structure to hold the state of the LEDs
struct LEDState {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
};

// Structure to hold the state of the potentiometers
struct PotentiometerState {
    uint16_t redPotValue;
    uint16_t greenPotValue;
    uint16_t bluePotValue;

    // Add previous values for filtering
    uint16_t redPotValueFiltered;
    uint16_t greenPotValueFiltered;
    uint16_t bluePotValueFiltered;
};


#endif // STATE_H
