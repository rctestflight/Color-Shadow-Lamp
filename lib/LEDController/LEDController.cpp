#include "LEDController.h"

LEDController::LEDController(
    int rPin, int gPin, int bPin,
    int rChannel, int gChannel, int bChannel,
    int freq, int res
) : redPin(rPin), 
    greenPin(gPin), 
    bluePin(bPin),
    redChannel(rChannel), 
    greenChannel(gChannel), 
    blueChannel(bChannel),
    frequency(freq), 
    resolution(res) 
{
}

void LEDController::begin() {
    // Configure LED PWM channels
    ledcSetup(redChannel, frequency, resolution);
    ledcSetup(greenChannel, frequency, resolution);
    ledcSetup(blueChannel, frequency, resolution);
    
    // Attach PWM channels to GPIO pins, but swap red and blue
    ledcAttachPin(redPin, blueChannel);    // Red pin gets blue channel
    ledcAttachPin(greenPin, greenChannel); // Green stays same
    ledcAttachPin(bluePin, redChannel);    // Blue pin gets red channel
    
    // Initialize all LEDs to off
    ledcWrite(redChannel, 0);
    ledcWrite(greenChannel, 0);
    ledcWrite(blueChannel, 0);

    loadPowerLimit();
}

void LEDController::loadPowerLimit() {
    preferences.begin("led", false);
    bool unlocked = preferences.getBool("unlocked", false);
    currentPowerLimit = unlocked ? UNLOCKED_POWER_LIMIT : LOCKED_POWER_LIMIT;
    preferences.end();
}

void LEDController::unlock() {
    preferences.begin("led", false);
    preferences.putBool("unlocked", true);
    preferences.end();
    currentPowerLimit = UNLOCKED_POWER_LIMIT;
}

void LEDController::resetToSafeMode() {
    preferences.begin("led", false);
    preferences.putBool("unlocked", false);
    preferences.end();
    currentPowerLimit = LOCKED_POWER_LIMIT;
}

void LEDController::applyPowerLimit(int& red, int& green, int& blue) {
    float totalPower = (red + green + blue) / (3.0f * 2047.0f);
    if (totalPower > currentPowerLimit) {
        float scale = currentPowerLimit / totalPower;
        red = static_cast<int>(red * scale);
        green = static_cast<int>(green * scale);
        blue = static_cast<int>(blue * scale);
    }
}

void LEDController::writePWM(int channel, int value) {
    value = static_cast<int>(value * currentPowerLimit);
    ledcWrite(channel, value);
}

void LEDController::setPWMDirectly(int red, int green, int blue) {

    // Constrain values first
    red = constrain(red, 0, 2047);
    green = constrain(green, 0, 2047);
    blue = constrain(blue, 0, 2000);

    bool updateRed = shouldUpdate(currentRed, red);
    bool updateGreen = shouldUpdate(currentGreen, green);
    bool updateBlue = shouldUpdate(currentBlue, blue);

    Serial.printf("DEBUG: Writing to channels - Red(ch%d): %d, Green(ch%d): %d, Blue(ch%d): %d\n",
    redChannel, red, greenChannel, green, blueChannel, blue);

    if (updateRed || updateGreen || updateBlue) {
        if (updateRed) {
            currentRed = red;
            writePWM(redChannel, red);
        }
        if (updateGreen) {
            currentGreen = green;
            writePWM(greenChannel, green);
        }
        if (updateBlue) {
            currentBlue = blue;
            writePWM(blueChannel, blue);
        }
    }
}

bool LEDController::shouldUpdate(int current, int new_value) {
    return abs(current - new_value) > updateThreshold;
}