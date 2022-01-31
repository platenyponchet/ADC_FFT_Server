#include "webserver.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

// Set LED GPIO
const int ledPin = LED_BUILTIN;
// Stores LED state
String ledState;

AsyncWebServer server(80);

uint16_t* i2sSamples;
uint16_t bufferIndex = 0;
const char *contentType = "application/json";


// Replaces placeholder with LED state value
String processor(const String& var){
    Serial.println(var);
    if(var == "STATE") {
        if(digitalRead(ledPin)) {
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

void webserverSetup(uint16_t* buffer) {
    i2sSamples = buffer;

    pinMode(ledPin, OUTPUT);

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

    // Route to set GPIO to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(ledPin, HIGH);    
        request->send(SPIFFS, "/index.html", String(),false, processor);
    });

    // Route to set GPIO to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        digitalWrite(ledPin, LOW);
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
                len += sprintf((char *)buffer, "[%4d", i2sSamples[bufferIndex]&0x0FFF);
                bufferIndex++;
            }
            while (len < (maxLen - 10) && bufferIndex < 512)
            {
                len += sprintf((char *)(buffer + len), ",%4d", i2sSamples[bufferIndex]&0x0FFF);
                bufferIndex++;
            }
            if (bufferIndex == 512) {
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