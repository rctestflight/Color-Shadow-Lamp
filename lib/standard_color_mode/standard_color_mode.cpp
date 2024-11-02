#include <Arduino.h>
#include "standard_color_mode.h"
#include "potentiometer.h"
#include "led_control.h"

void initStandardColorMode() {
    // Any initialization specific to Standard Color Mode can go here.
}

void handleStandardColorMode(PotentiometerState& potState, LEDState& ledState) {
    // Read and filter potentiometer values
    readPotentiometers(potState);

    // Print raw and filtered values
    Serial.print("Raw ADC Values - Red: ");
    Serial.print(potState.redPotValue);
    Serial.print(", Green: ");
    Serial.print(potState.greenPotValue);
    Serial.print(", Blue: ");
    Serial.println(potState.bluePotValue);

    // Map potentiometer values to LED brightness (0-2047)
    ledState.red = map(potState.redPotValue, 0, 4095, 0, 2047);
    ledState.green = map(potState.greenPotValue, 0, 4095, 0, 2047);
    ledState.blue = map(potState.bluePotValue, 0, 4095, 0, 2047);

    

    // Print mapped LED values
    Serial.print("LED Values - Red: ");
    Serial.print(ledState.red);
    Serial.print(", Green: ");
    Serial.print(ledState.green);
    Serial.print(", Blue: ");
    Serial.println(ledState.blue);

    // Update LEDs
    setLEDColor(ledState);

    delay(10); // Adjust delay as needed
}



