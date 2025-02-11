#include <Arduino.h>
#include "LEDController.h"
#include "LTTController.h"
#include "WiFiManager.h"
#include "State.h"

const int RED_PIN = 5;
const int GREEN_PIN = 6;
const int BLUE_PIN = 7;

const int POT_RED_PIN = 4;
const int POT_GREEN_PIN = 3;
const int POT_BLUE_PIN = 0;

LEDController ledController(
    RED_PIN, GREEN_PIN, BLUE_PIN,
    0, 1, 2);

LTTController lttController(ledController);
WiFiManager wifiManager(ledController);
StateHandler stateHandler(ledController);

int readAveragedADC(int pin, int samples = 4)
{
  int32_t sum = 0;
  for (int i = 0; i < samples; i++)
  {
    sum += analogReadMilliVolts(pin);
  }
  return sum / samples;
}

void setup()
{
  Serial.begin(115200);
  ledController.begin();
  stateHandler.begin();

  analogSetAttenuation(ADC_2_5db);
  analogSetPinAttenuation(POT_RED_PIN, ADC_2_5db);
  analogSetPinAttenuation(POT_GREEN_PIN, ADC_2_5db);
  analogSetPinAttenuation(POT_BLUE_PIN, ADC_2_5db);
}

void loop()
{
  static unsigned long lastUpdate = 0;
  const unsigned long UPDATE_INTERVAL = 20;

  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL)
  {
    lastUpdate = currentMillis;

    stateHandler.update();

    if (stateHandler.getCurrentMode() == OperationMode::WIFI)
    {
      wifiManager.update();
    }

    int pot1 = readAveragedADC(POT_RED_PIN);
    int pot2 = readAveragedADC(POT_GREEN_PIN);
    int pot3 = readAveragedADC(POT_BLUE_PIN);

    pot1 = map(constrain(pot1, 5, 950), 5, 950, 0, 2047); // Left pot (meant for Red)
    pot2 = map(constrain(pot2, 5, 950), 5, 950, 0, 2047); // Middle pot (meant for Green)
    pot3 = map(constrain(pot3, 5, 950), 5, 950, 0, 2047); // Right pot (meant for Blue)

    static bool wasInWiFiMode = false;
    bool isInWiFiMode = stateHandler.getCurrentMode() == OperationMode::WIFI;

    if (isInWiFiMode && !wasInWiFiMode)
    {
      wifiManager.begin();
      ledController.checkAndUpdatePowerLimit();
    }
    else if (!isInWiFiMode && wasInWiFiMode)
    {
      wifiManager.stop();
      ledController.setPWMDirectly(0, 0, 0);
      ledController.checkAndUpdatePowerLimit();
    }
    wasInWiFiMode = isInWiFiMode;

    switch (stateHandler.getCurrentMode())
    {
    case OperationMode::RGB:
      // pot1 is LEFT (physically red), pot3 is RIGHT (physically blue)
      // since the LED pins are now swapped in begin(), we need to swap these too
      ledController.setPWMDirectly(pot3, pot2, pot1); // Swap values to match physical layout
      break;
    case OperationMode::LTT:
      lttController.updateLTT(pot1, pot2, pot3);
      break;
    case OperationMode::OFF:
    case OperationMode::WIFI:
      // LED control happens via WiFi in WIFI mode
      break;
    }
  }
  delay(2);
}