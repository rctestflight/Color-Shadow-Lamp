#include "LTTController.h"

void LTTController::lttToRgb(int luminance, int temperature, int tintVal, int& r, int& g, int& b) {
    float l = luminance / 2047.0f;
    float t = temperature / 2047.0f;
    float tv = tintVal / 2047.0f;
    
    float baseR = 1.0f - t * 0.5f;
    float baseB = 0.5f + t * 0.5f;
    float baseG = 0.8f * (0.5f + tv * 0.5f);
    
    r = static_cast<int>(baseR * l * 2047);
    g = static_cast<int>(baseG * l * 2047);
    b = static_cast<int>(baseB * l * 2047);
    
    r = constrain(r, 0, 2047);
    g = constrain(g, 0, 2047);
    b = constrain(b, 0, 2047);
}

void LTTController::updateLTT(int luminance, int temperature, int tint) {
    int r, g, b;
    lttToRgb(luminance, temperature, tint, r, g, b);
    ledController.setPWMDirectly(r, g, b);
}