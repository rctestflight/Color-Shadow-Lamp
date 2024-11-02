#ifndef INIT_H
#define INIT_H

#include "state.h"

// Initializes all system components.
// Parameters:
//   - potState: Reference to the PotentiometerState struct.
//   - ledState: Reference to the LEDState struct.
void initializeSystem(PotentiometerState& potState, LEDState& ledState);

#endif // INIT_H
