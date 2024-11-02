#include "init.h"
#include "led_control.h"
#include "potentiometer.h"
#include "standard_color_mode.h"

void initializeSystem(PotentiometerState& potState, LEDState& ledState) {
    // Initialize hardware components
    initLEDs();
    initPotentiometers();

    // Initialize Standard Color Mode
    initStandardColorMode();

    // Initialize the state structs
    potState.redPotValue = potState.greenPotValue = potState.bluePotValue = 0;
    ledState.red = ledState.green = ledState.blue = 0;
}
