#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <Preferences.h>

class LEDController {
private:
    const int redPin;
    const int greenPin;
    const int bluePin;
    const int frequency;
    const int resolution;
    const int redChannel;
    const int greenChannel;
    const int blueChannel;
    int currentRed = 0;
    int currentGreen = 0;
    int currentBlue = 0;
    const int updateThreshold = 4;
    bool shouldUpdate(int current, int new_value);

    static constexpr float LOCKED_POWER_LIMIT = 0.3f;   // 20% power
    static constexpr float UNLOCKED_POWER_LIMIT = 0.6f; // 60% power
    float currentPowerLimit;
    Preferences preferences;
    
    void applyPowerLimit(int& red, int& green, int& blue);
    void loadPowerLimit();
    void writePWM(int channel, int value);

public:
    LEDController(
        int redPin, int greenPin, int bluePin,
        int redChannel = 0, int greenChannel = 1, int blueChannel = 2,
        int frequency = 19000, int resolution = 11
    );
    void begin();
    void setPWMDirectly(int red, int green, int blue);
    void getPWMValues(int& red, int& green, int& blue) {
        red = currentRed;
        green = currentGreen;
        blue = currentBlue;
    }
    bool isUnlocked() const { return currentPowerLimit > LOCKED_POWER_LIMIT; }
    void unlock();
    void resetToSafeMode();
};

#endif