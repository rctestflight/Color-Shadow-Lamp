#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include "LEDController.h"

class WiFiManager
{
private:
    AsyncWebServer server;
    LEDController &ledController;
    const char *ssid = "Color_Shadow";
    const char *password = "4";
    unsigned long lastUpdate = 0;
    const unsigned long MIN_UPDATE_INTERVAL = 5;

    void handleRoot(AsyncWebServerRequest *request)
    {
        Serial.println("Serving index.html");
        request->send(SPIFFS, "/index.html", "text/html");
    }

    void handleIroMin(AsyncWebServerRequest *request)
    {
        Serial.println("Serving iro.min.js");
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/iro.min.js", "text/javascript");
        response->addHeader("Cache-Control", "max-age=31536000");
        response->addHeader("Expires", "Thu, 31 Dec 2037 23:59:59 GMT");
        request->send(response);
    }

    void handleIroScript(AsyncWebServerRequest *request)
    {
        Serial.println("Serving iro_script.js");
        request->send(SPIFFS, "/iro_script.js", "text/javascript");
    }

    void handleLockStatus(AsyncWebServerRequest *request)
    {
        Serial.println("Lock status requested");
        bool isUnlocked = ledController.isUnlocked();
        Serial.printf("Current lock status: %s\n", isUnlocked ? "unlocked" : "locked");
        String response = isUnlocked ? "{\"unlocked\":true}" : "{\"unlocked\":false}";
        request->send(200, "application/json", response);
    }

    void handleUnlock(AsyncWebServerRequest *request)
    {
        Serial.println("Unlock requested");
        ledController.unlock();
        Serial.println("Unlock complete");
        request->send(200, "text/plain", "OK");
    }

    void handleReset(AsyncWebServerRequest *request)
    {
        Serial.println("Reset requested");
        ledController.resetToSafeMode();
        Serial.println("Reset complete");
        request->send(200, "text/plain", "OK");
    }

    void handleRGB(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
    {
        if (request->hasParam("r", true) && request->hasParam("g", true) && request->hasParam("b", true))
        {
            int r = request->getParam("r", true)->value().toInt();
            int g = request->getParam("g", true)->value().toInt();
            int b = request->getParam("b", true)->value().toInt();

            // Debug print
            Serial.printf("Received RGB request: r=%d, g=%d, b=%d\n", r, g, b);

            int mappedR = map(constrain(r, 0, 255), 0, 255, 0, 2047);
            int mappedG = map(constrain(g, 0, 255), 0, 255, 0, 2047);
            int mappedB = map(constrain(b, 0, 255), 0, 255, 0, 2047);

            Serial.printf("Mapped RGB values: r=%d, g=%d, b=%d\n", mappedR, mappedG, mappedB);

            ledController.setPWMDirectly(mappedR, mappedG, mappedB);
            request->send(200, "text/plain", "OK");
        }
        else
        {
            Serial.println("Invalid RGB parameters received");
            request->send(400, "text/plain", "Bad Request");
        }
    }

public:
    WiFiManager(LEDController &controller) : server(80), ledController(controller) {}

    void begin()
    {
        if (!SPIFFS.begin(true))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }

        // 1. Register WiFi event handler FIRST
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
            Serial.printf("[WiFi] Event: %d\n", event);
        });

        // Disable WiFi power save for better responsiveness
        WiFi.setSleep(false);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);

        // Start AP before server
        WiFi.softAPdisconnect(true);
        delay(100);
        WiFi.softAP(ssid, password);
        delay(500); // Crucial for AP stabilization

        // Verify AP IP
        IPAddress apIP = WiFi.softAPIP();
        if (apIP == IPAddress(0, 0, 0, 0))
        {
            Serial.println("AP Failed - Rebooting");
            ESP.restart();
        }

        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

        server.on("/", HTTP_GET, std::bind(&WiFiManager::handleRoot, this, std::placeholders::_1));
        server.on("/iro.min.js", HTTP_GET, std::bind(&WiFiManager::handleIroMin, this, std::placeholders::_1));
        server.on("/iro_script.js", HTTP_GET, std::bind(&WiFiManager::handleIroScript, this, std::placeholders::_1));
        server.on("/lockStatus", HTTP_GET, std::bind(&WiFiManager::handleLockStatus, this, std::placeholders::_1));
        server.on("/unlock", HTTP_POST, std::bind(&WiFiManager::handleUnlock, this, std::placeholders::_1));
        server.on("/reset", HTTP_POST, std::bind(&WiFiManager::handleReset, this, std::placeholders::_1));

        // In WiFiManager.h constructor
        server.on("/postRGB", HTTP_POST, [this](AsyncWebServerRequest *request)
                  {
            // Check for all 3 parameters first
            if(!request->hasParam("r", true) || !request->hasParam("g", true) || !request->hasParam("b", true)) {
                request->send(400, "text/plain", "Missing parameters");
                return;
            }

            // Get and constrain values
            int r = request->getParam("r", true)->value().toInt();
            int g = request->getParam("g", true)->value().toInt();
            int b = request->getParam("b", true)->value().toInt();
            
            r = constrain(r, 0, 255);
            g = constrain(g, 0, 255);
            b = constrain(b, 0, 255);

            // Convert to 11-bit PWM range (0-2047)
            int pwm_r = map(r, 0, 255, 0, 2047);
            int pwm_g = map(g, 0, 255, 0, 2047);
            int pwm_b = map(b, 0, 255, 0, 2047);

            // Debug output
            Serial.printf("[WiFi] Received RGB: %d,%d,%d -> PWM: %d,%d,%d\n", 
                        r, g, b, pwm_r, pwm_g, pwm_b);

            // Set LEDs - account for physical pin swap
            ledController.setPWMDirectly(pwm_r, pwm_g, pwm_b);
    
        request->send(200, "text/plain", "OK"); });

        server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(404); });

        try
        {
            server.begin();
            Serial.println("Async HTTP server started successfully");
        }
        catch (...)
        {
            Serial.println("Failed to start server - attempting restart");
            delay(1000);
            ESP.restart();
        }
    }

    void update()
    {
        // Not needed with AsyncWebServer
    }

    void stop()
    {
        server.end();
        delay(100);
        WiFi.softAPdisconnect(true);
        delay(100);
        Serial.println("WiFi and server stopped");
    }
};