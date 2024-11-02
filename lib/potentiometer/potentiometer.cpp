#include "potentiometer.h"
#include <Arduino.h>

// Define potentiometer pins (adjust according to your hardware setup)
const uint16_t POT_PINS[3] = {4, 3, 0}; // Example ADC pins for pots

void initPotentiometers() {
    // Initialize potentiometer pins as inputs and set attenuation
    for (uint8_t i = 0; i < 3; i++) {
        pinMode(POT_PINS[i], INPUT);
        analogSetPinAttenuation(POT_PINS[i], ADC_11db); // Change to 11dB attenuation
    }
}

void readPotentiometers(PotentiometerState& potState) {
    const float alpha = 0.1; // Smoothing factor (adjust as needed)

    uint16_t rawRed = analogRead(POT_PINS[0]);
    uint16_t rawGreen = analogRead(POT_PINS[1]);
    uint16_t rawBlue = analogRead(POT_PINS[2]);

    // Initialize filtered values on first run
    static bool firstRun = true;
    if (firstRun) {
        potState.redPotValueFiltered = rawRed;
        potState.greenPotValueFiltered = rawGreen;
        potState.bluePotValueFiltered = rawBlue;
        firstRun = false;
    } else {
        // Apply exponential moving average
        potState.redPotValueFiltered = alpha * rawRed + (1 - alpha) * potState.redPotValueFiltered;
        potState.greenPotValueFiltered = alpha * rawGreen + (1 - alpha) * potState.greenPotValueFiltered;
        potState.bluePotValueFiltered = alpha * rawBlue + (1 - alpha) * potState.bluePotValueFiltered;
    }

    // Update raw values
    potState.redPotValue = potState.redPotValueFiltered;
    potState.greenPotValue = potState.greenPotValueFiltered;
    potState.bluePotValue = potState.bluePotValueFiltered;
}

