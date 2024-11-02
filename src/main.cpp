/*
Copyright (c) 2024 Stratus LEDS

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <WiFi.h>
//#include <WebSocketsServer.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// to simplify code all RGB values are stored in 12 bit range 0-4095
// they are then converted 8->12, 12->11, 12->8 etc as needed using bitshifts or maps

const char compile_date[] = __DATE__ " " __TIME__;

// Network settings vvvvvvvvvvvvvvvvvvvvvvvvvvv

const char *ssid = "colorshadow";
const char *password = "colorshadow";

IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

const int POT_MIN_SCALE = 10;     //lower and upper bounds so pots can hit full scale easily
const int POT_MAX_SCALE = 620;    //lower and upper bounds so pots can hit full scale easily
const int LED_MIN_SCALE = 0;      // 
const int LED_MAX_SCALE = 4095;   // 2^12 -1
const float LED_TRIM_RED = 0.6;    // 0.5 half power,  1.0 full power
const float LED_TRIM_GREEN = 0.6;  // 0.5 half power,  1.0 full power
const float LED_TRIM_BLUE = 0.6;   // 0.5 half power,  1.0 full power


// Pin assignment
const int potPin0 = 4;
const int potPin1 = 3;
const int potPin2 = 0;
const int vinPin0 = 1;

const int swPin0 = 9;

const int ledPWMPin0 = 5;
const int ledPWMPin1 = 6;
const int ledPWMPin2 = 7;

// Variables to store RGB values, default to 10 so that the LED is dimmly lit when started.
int r = 10;
int g = 10;
int b = 10;

// variables for storing adc and switch values
int potValue0 = 0;
int potValue1 = 0;
int potValue2 = 0;
int vinValue0 = 0;
int swValue0 = 0;

typedef enum {
    STATE_POT_RGB = 0,
    STATE_POT_HSL = 1,
    STATE_WIFI = 2
} State_type;

State_type currState = STATE_POT_RGB;


// LED Control Setting vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv



//Set PWM freqency, channels and resolution
const int freq = 10000;
const int resolution = 11;
const int ledChannel0 = 0;
const int ledChannel1 = 1;
const int ledChannel2 = 2;



typedef struct rgb {
  float r, g, b;
} RGB;


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
AsyncWebServer server(80);


/*
 * Converts an HUE to r, g or b.
 * returns float in the set [0, 1].
 */
float hue2rgb(float p, float q, float t) {

  if (t < 0) 
    t += 1;
  if (t > 1) 
    t -= 1;
  if (t < 1./6) 
    return p + (q - p) * 6 * t;
  if (t < 1./2) 
    return q;
  if (t < 2./3)   
    return p + (q - p) * (2./3 - t) * 6;
    
  return p;
  
}



//
// Converts an HSL color value to RGB. Conversion formula
// adapted from http://en.wikipedia.org/wiki/HSL_color_space.
// Assumes h, s, and l are contained in the set [0, 1] and
// returns RGB in the set [0, 1].
//

RGB hsl2rgb(float h, float s, float l) {

  RGB result;
  
  if(0 == s) {
    result.r = result.g = result.b = l; // achromatic
  }
  else {
    float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;
    result.r = hue2rgb(p, q, h + 1./3);
    result.g = hue2rgb(p, q, h);
    result.b = hue2rgb(p, q, h - 1./3);
  }

  return result;

}

// main call to write to LED.  use 12 bits internal value.  use 11 bits for ESP32 PWM.  This is to achieve higher refresh rate (15khz).
void writeToLED12(int red, int green, int blue) {

  // only update if different than last call to this function
  // BUGFIX: this stops pwm flicker from updating register while generating PWM
 static int old_red, old_green, old_blue;

 if (red == old_red && green == old_green && blue == old_blue) {
  return; // nothing to update
 }

 old_red = red;
 old_green = green;
 old_blue = blue;

  red = red * LED_TRIM_RED;
  green = green * LED_TRIM_GREEN;
  blue = blue * LED_TRIM_BLUE;

  red = constrain(red, LED_MIN_SCALE, LED_MAX_SCALE);
  green = constrain(green, LED_MIN_SCALE, LED_MAX_SCALE);
  blue = constrain(blue, LED_MIN_SCALE, LED_MAX_SCALE);

  // 12 bit to 11 bit
  red = red >> 1;
  green = green >> 1;
  blue = blue >> 1; 
  
  ledcWrite(ledChannel0, blue);
  ledcWrite(ledChannel1, green);
  ledcWrite(ledChannel2, red);
  
}

// bit shift and then call the 12 bit function
void writeToLED8(int red, int green, int blue) {
  writeToLED12(red<<4, green<<4, blue<<4);
}


void setup() {
  Serial.begin(115200);

  // configure ADC attenuation
  analogSetAttenuation(ADC_2_5db);
  analogSetPinAttenuation(potPin0, ADC_2_5db);
  analogSetPinAttenuation(potPin1, ADC_2_5db);
  analogSetPinAttenuation(potPin2, ADC_2_5db);
  analogSetPinAttenuation(vinPin0, ADC_0db);

  // configure SW button input
  pinMode(swPin0, INPUT);


  // setting PWM properties
  ledcSetup(ledChannel0, freq, resolution);
  ledcSetup(ledChannel1, freq, resolution);
  ledcSetup(ledChannel2, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(ledPWMPin0, ledChannel0);
  ledcAttachPin(ledPWMPin1, ledChannel1);
  ledcAttachPin(ledPWMPin2, ledChannel2);

  // init starting value to 0
  ledcWrite(ledChannel0, 0);
  ledcWrite(ledChannel1, 0);
  ledcWrite(ledChannel2, 0);

  //pinMode(buttonPin, INPUT_PULLUP);
  
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


  // Set ESP32 to Access Point mode
  WiFi.mode(WIFI_AP);

  // Set Access Point name and password
  WiFi.softAP(ssid, password);

  // Set IP address of the Access Point
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Initialize SPIFFS file system
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  if (SPIFFS.exists("/index.html")) {
    Serial.println("index.html file found");
  } else {
    Serial.println("index.html file not found");
  }

  if (SPIFFS.exists("/iro_script.js")) {
    Serial.println("/iro_script.js file found");
  } else {
    Serial.println("/iro_script.js file not found");
  }

  if (SPIFFS.exists("/iro.min.js")) {
    Serial.println("/iro.min.js file found");
  } else {
    Serial.println("/iro.min.js file not found");
  }

  // Route to load the color picker HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Color picker page requested");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Route to load the iro.min.js file
  server.on("/iro.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("iro.min.js requested");
    request->send(SPIFFS, "/iro.min.js", "text/javascript");
  });

  // Route to load the iro_script.js file
  server.on("/iro_script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("iro_script.js requested");
    request->send(SPIFFS, "/iro_script.js", "text/javascript");
  });

  // Route to handle POST requests containing RGB values
  server.on(
    "/postRGB",
    HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Concatenate the data chunk into a String
      static String body;
      body.concat(String((char *)data, len));

      // If all data has been received, parse the RGB values and send a response
      if (index + len == total) {
        // Parse the RGB values from the request body
        //int r, g, b;
        sscanf(body.c_str(), "r=%d&g=%d&b=%d", &r, &g, &b);

        // Print the RGB values to the serial monitor
        Serial.print("RGB values: ");
        Serial.print(r);
        Serial.print(", ");
        Serial.print(g);
        Serial.print(", ");
        Serial.println(b);

        // change state to control by wifi
        currState = STATE_WIFI;
        writeToLED8(r,g,b);

        // Send a response back to the client
        request->send(200, "text/plain", "RGB values received");

        // Reset the body String
        body = "";
      }
    });

  // Start AsyncWebServer
  server.begin();


  // Print IP address of the Access Point
  Serial.println(WiFi.softAPIP());

 // blink
//   writeToLED8(255, 0, 0);
//   delay(250);
//   writeToLED8(0, 255, 0);
//   delay(250);
//   writeToLED8(0, 0, 255);
//   delay(250);
//   writeToLED8(255, 255, 255);
//   delay(250);
//   writeToLED8(0, 0, 0);
//   delay(250);
//   delay(0);

  Serial.print("Compile Date: ");
  Serial.println(compile_date);
}

RGB CCT_to_RGB(float CCT, float duv) {
    float x, y, z;
    float X, Y, Z;
    float R, G, B;

    // Convert CCT to xy coordinates
    if (CCT >= 1667 && CCT <= 4000) {
        x = -0.2661239f * 1e9f / (CCT * CCT * CCT) - 0.2343580f * 1e6f / (CCT * CCT) + 0.8776956f * 1e3f / CCT + 0.179910f;
    } else {
        x = -3.0258469f * 1e9f / (CCT * CCT * CCT) + 2.1070379f * 1e6f / (CCT * CCT) + 0.2226347f * 1e3f / CCT + 0.240390f;
    }
    y = -1.1063814f * x * x * x - 1.34811020f * x * x + 2.18555832f * x - 0.20219683f;

    // Calculate z
    z = 1 - x - y;

    // Convert xyY to XYZ
    Y = 1.0f;
    X = (Y / y) * x;
    Z = (Y / y) * z;

    // Convert XYZ to RGB
    R =  3.2404542f * X - 1.5371385f * Y - 0.4985314f * Z;
    G = -0.9692660f * X + 1.8760108f * Y + 0.0415560f * Z;
    B =  0.0556434f * X - 0.2040259f * Y + 1.0572252f * Z;

    // Adjust for tint
    R += 0.0241f * duv;
    G -= 0.1245f * duv;

    // Clip values to [0, 1] range
    R = (R < 0) ? 0 : ((R > 1) ? 1 : R);
    G = (G < 0) ? 0 : ((G > 1) ? 1 : G);
    B = (B < 0) ? 0 : ((B > 1) ? 1 : B);

    RGB result = {R, G, B};
    return result;
}

RGB Lab2RGB(float L, float a, float b) {
    RGB result;
    // Constants
    float T1 = 0.008856;
    float T2 = 0.206893;
    float D65_X = 0.950456;
    float D65_Z = 1.088754;
    float MAT[3][3] = {
        { 3.240479, -1.537150, -0.498535 },
        { -0.969256,  1.875992,  0.041556 },
        { 0.055648, -0.204043,  1.057311 }
    };

    // Compute Y
    float fY = pow((L + 16.0) / 116.0, 3);
    int YT = fY > T1;
    fY = (!YT) * (L / 903.3) + YT * fY;
    float Y = fY;

    // Compute X
    float fX = a / 500.0 + fY;
    int XT = fX > T2;
    float X = (XT * pow(fX, 3) + (!XT) * ((fX - 16.0 / 116.0) / 7.787));

    // Compute Z
    float fZ = fY - b / 200.0;
    int ZT = fZ > T2;
    float Z = (ZT * pow(fZ, 3) + (!ZT) * ((fZ - 16.0 / 116.0) / 7.787));

    // Normalize for D65 white point
    X *= D65_X;
    Z *= D65_Z;

    // XYZ to RGB
    float XYZ[3] = { X, Y, Z };
    for (int i = 0; i < 3; ++i) {
        float value = 0;
        for (int j = 0; j < 3; ++j) {
            value += MAT[i][j] * XYZ[j];
        }
        value = fmin(1, fmax(0, value)); // Clamp value to [0, 1]
        if (i == 0)
            result.r = value;
        else if (i == 1)
            result.g = value;
        else if (i == 2)
            result.b = value;
    }

    return result;
}

void loop() {
  static int tick = 0;
  tick = tick + 1;

  // take 8 readings of each input and average them together
  int raw0 = 0;
  int raw1 = 0;
  int raw2 = 0;
  for (int i=0;i<8;i++) {
    raw0 = raw0-(raw0>>3); // >>3 equiv to raw0/8.0
    raw1 = raw1-(raw1>>3);
    raw2 = raw2-(raw2>>3);
    
    raw0 = raw0 + analogReadMilliVolts(potPin0);
    raw1 = raw1 + analogReadMilliVolts(potPin1);
    raw2 = raw2 + analogReadMilliVolts(potPin2);
  }
  raw0 = raw0>>3; // >>3 equiv to raw0/8.0
  raw1 = raw1>>3;
  raw2 = raw2>>3;

  // read analog dial inputs and scale range 
  int potReading0  = constrain(raw0,POT_MIN_SCALE,POT_MAX_SCALE);
  int potReading1  = constrain(raw1,POT_MIN_SCALE,POT_MAX_SCALE);
  int potReading2  = constrain(raw2,POT_MIN_SCALE,POT_MAX_SCALE);
  
  potReading0  = map(potReading0,POT_MIN_SCALE,POT_MAX_SCALE,LED_MIN_SCALE,LED_MAX_SCALE);
  potReading1  = map(potReading1,POT_MIN_SCALE,POT_MAX_SCALE,LED_MIN_SCALE,LED_MAX_SCALE);
  potReading2  = map(potReading2,POT_MIN_SCALE,POT_MAX_SCALE,LED_MIN_SCALE,LED_MAX_SCALE);

  


  // Reading potentiometer value with an avg
  potValue0 = (7.0/8.0)*potValue0 + (1.0/8.0)*potReading0;
  potValue1 = (7.0/8.0)*potValue1 + (1.0/8.0)*potReading1;
  potValue2 = (7.0/8.0)*potValue2 + (1.0/8.0)*potReading2;
  vinValue0 = analogReadMilliVolts(vinPin0);
  swValue0 = digitalRead(swPin0);

  ///potValue0 = 350;
  //potValue1 = 350;
  //potValue2 = 350;

  //Serial.printf("raw0: %i potReading0: %i, potValue0: %i\n", raw0, potReading0, potValue0);



  switch(currState)
  {
    case STATE_POT_RGB:
      if (swValue0 == 0) { // button press detected
        currState = STATE_POT_HSL;
        do {
          delay(50); 
          } while (digitalRead(swPin0) == 0);  //debounce
        //Serial.println("Switching to CTB mode");
      }

      writeToLED12(potValue0, potValue1, potValue2);
      break;

      
    case STATE_POT_HSL:
    {
      // color temp brightness
    
      if (swValue0 == 0) { // button press detected
        currState = STATE_POT_RGB;
        do { delay(50); } while (digitalRead(swPin0) == 0);  //debounce
        //Serial.println("Switching to RGB mode");
      }
      
      //writeToLED12(LED_MAX_SCALE, LED_MAX_SCALE, LED_MAX_SCALE);
      RGB result;
      result = hsl2rgb((float)potValue0 / LED_MAX_SCALE, (float)potValue1 / LED_MAX_SCALE, (float)potValue2 / LED_MAX_SCALE);
      writeToLED12(result.r*LED_MAX_SCALE, result.g*LED_MAX_SCALE, result.b*LED_MAX_SCALE);


      //float CCT = 5000.0; // Example CCT
      //float duv = 0.003; // Example tint

      //CCT = map(potValue0, LED_MIN_SCALE, LED_MAX_SCALE, 2000, 6500);
      //duv = map(potValue1, LED_MIN_SCALE, LED_MAX_SCALE, 0, 10000);
      //duv = (duv-5000) / 10000.0;
      

      //RGB result = CCT_to_RGB(CCT, duv);
      //Serial.printf("%i %i %i %i %i %.1fV\n", potValue0, potValue1, potValue2, vinValue0, swValue0, (.032258 * vinValue0));

      //float l = map(potValue0, LED_MIN_SCALE, LED_MAX_SCALE, 0, 100);
      //float a = map(potValue1, LED_MIN_SCALE, LED_MAX_SCALE, 0, 220);
      //float b = map(potValue2, LED_MIN_SCALE, LED_MAX_SCALE, 0, 220);
      //a = a - 110;
      //b = b - 110;


//
      //float l = map(potValue0, LED_MIN_SCALE, LED_MAX_SCALE, 0, 100);
      //float a = map(potValue1, LED_MIN_SCALE, LED_MAX_SCALE, 0, 50);
      //float b = map(potValue2, LED_MIN_SCALE, LED_MAX_SCALE, 0, 100);
      //a = a - 25;
      //b = b - 50;


      //RGB result = Lab2RGB(l,a,b);
      
      //Serial.printf("%f %f %f %f %f %f \n", l,a,b, result.r, result.g, result.b);
      
      
      //writeToLED12(result.r*LED_MAX_SCALE, result.g*LED_MAX_SCALE, result.b*LED_MAX_SCALE);
      
      break;
     } 
     
    case STATE_WIFI:
      if (swValue0 == 0) { // button press detected
        currState = STATE_POT_RGB;
        do { delay(50); } while (digitalRead(swPin0) == 0);  //debounce
        //Serial.println("Switching to RGB mode");
      }
      break;

      
    default:
      break;
  }


  
  
  if (tick % 50 == 0) {
    Serial.printf("R: %i G: %i B: %i SW0: %i , VIN: %.2fV\n", potValue0, potValue1, potValue2, swValue0, (	0.03222 * vinValue0)- 0.1211);
  }
  
  delay(20);

  
}


