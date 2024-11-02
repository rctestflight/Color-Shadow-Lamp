#ifndef STANDARD_COLOR_MODE_H
#define STANDARD_COLOR_MODE_H

#include "state.h"

// Initializes Standard Color Mode.
void initStandardColorMode();

// Handles the logic for Standard Color Mode.
// Parameters:
//   - potState: Reference to the PotentiometerState struct.
//   - ledState: Reference to the LEDState struct.
void handleStandardColorMode(PotentiometerState& potState, LEDState& ledState);

#endif // STANDARD_COLOR_MODE_H
