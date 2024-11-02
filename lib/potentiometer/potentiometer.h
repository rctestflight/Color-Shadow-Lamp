#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include <stdint.h>
#include "state.h"

// Initializes the potentiometer hardware.
void initPotentiometers();

// Reads the values from all potentiometers and updates the PotentiometerState struct.
// Parameters:
//   - potState: Reference to a PotentiometerState struct to store the values.
void readPotentiometers(PotentiometerState& potState);


#endif // POTENTIOMETER_H
