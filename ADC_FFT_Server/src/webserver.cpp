// Plateny de Brito Ponchet

#include <math.h>
#include "webserver.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define FFT_SAMPLES     64
#define PWM_FREQ        429
#define PWM_RESOLUTION  8
#define PWM_DUTY_CICLE  50

// Set LED GPIO
const int pwmPin = LED_BUILTIN;
// Stores LED state
String ledState;

AsyncWebServer server(80);

double* fftResult_;
uint16_t bufferIndex = 0;
const char *contentType = "application/json";

// Replaces placeholder with LED state value
String processor(const String& var){
    Serial.println(var);
    if(var == "STATE") {
        if(digitalRead(pwmPin)) {
            ledState = "ON";
        }
        else {
            ledState = "OFF";
        }
        Serial.print(ledState);
        return ledState;
    }
    return String();
}

void webserverSetup(double* buffer) {
    fftResult_ = buffer;

    pinMode(pwmPin, OUTPUT);
    ledcAttachPin(pwmPin, 0);
    ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(0, (uint32_t)(pow(2,PWM_RESOLUTION)*PWM_DUTY_CICLE/100.0));

    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    server.serveStatic("/style.css", SPIFFS, "/style.css");
    server.serveStatic("/chart.js", SPIFFS, "/chart.js");
    server.serveStatic("/jquery.js", SPIFFS, "/jquery.js");

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(),false, processor);
    });

    server.on("/array", HTTP_GET, [](AsyncWebServerRequest * request)
    {
        bufferIndex = 0;
        AsyncWebServerResponse* response = request->beginChunkedResponse(contentType,
                                            [](uint8_t* buffer, size_t maxLen, size_t index)
        {
            maxLen = maxLen >> 1;
            size_t len = 0;

            if (bufferIndex == 0)
            {
                len += sprintf((char *)buffer, "[%.2f", fftResult_[bufferIndex]);
                bufferIndex++;
            }
            while (len < (maxLen - 10) && bufferIndex < FFT_SAMPLES)
            {
                len += sprintf((char *)(buffer + len), ",%.2f", fftResult_[bufferIndex]);
                bufferIndex++;
            }
            if (bufferIndex == FFT_SAMPLES) {
                len += sprintf((char *)(buffer + len), "]");
                bufferIndex++;
            }

            return len;
        });
        request->send(response);
    });

    // Start server
    server.begin();
}