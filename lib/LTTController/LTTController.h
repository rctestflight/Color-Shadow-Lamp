#ifndef LTT_CONTROLLER_H
#define LTT_CONTROLLER_H

#include "LEDController.h"

class LTTController {
private:
    LEDController& ledController;
    void lttToRgb(int luminance, int temperature, int tintVal, int& r, int& g, int& b);

public:
    LTTController(LEDController& controller) : ledController(controller) {}
    void updateLTT(int luminance, int temperature, int tint);
};

#endif