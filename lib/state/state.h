#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "LEDController.h"

enum class OperationMode {
    RGB,
    LTT,
    WIFI,
    OFF,
};

class StateHandler {
private:
    static constexpr int BUTTON_PIN = 9;
    static constexpr int DEBOUNCE_TIME = 15;
    
    OperationMode currentMode;
    unsigned long lastButtonPress;
    bool buttonWasPressed;
    LEDController &ledController;

public:
    StateHandler(LEDController &controller)
        : currentMode(OperationMode::RGB), lastButtonPress(0), buttonWasPressed(false), ledController(controller) {}

    void begin() {
        currentMode = OperationMode::RGB;
        pinMode(BUTTON_PIN, INPUT);
    }

    OperationMode getCurrentMode() const {
        return currentMode;
    }

    void update() {
        bool buttonIsPressed = (digitalRead(BUTTON_PIN) == 0);
        
        if (buttonIsPressed && !buttonWasPressed) {
            unsigned long now = millis();
            if (now - lastButtonPress >= DEBOUNCE_TIME) {
                lastButtonPress = now;
                
                switch (currentMode) {
                    case OperationMode::OFF:
                        currentMode = OperationMode::RGB;
                        break;
                    case OperationMode::RGB:
                        currentMode = OperationMode::LTT;
                        break;
                    case OperationMode::LTT:
                        currentMode = OperationMode::WIFI;
                        break;
                    case OperationMode::WIFI:
                        currentMode = OperationMode::OFF;
                        break;
                }
            }
        }
        buttonWasPressed = buttonIsPressed;
    }
};

#endif