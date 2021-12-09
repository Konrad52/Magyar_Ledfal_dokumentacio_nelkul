#ifndef SIMPLE_SERVER_H
#define SIMPLE_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <AsyncTCP.h>
#endif

const String PREFIX = "[SimpleServer]";
const String INFO   = "[*]";
const String ERROR  = "[!]";

class StoredValue { 
    private:
        char name[17];
        int  type    ; // 0 - Int | 1 - Float | 2 - String
        bool readOnly;

    public:
        void* value;
        StoredValue(String name, void* value, int type, bool readOnly);

        String getName();
        int    getType();
        bool   isReadOnly();
};

StoredValue StoredInt(String name, void* value, bool readOnly);

StoredValue StoredFloat(String name, void* value, bool readOnly);

StoredValue StoredString(String name, void* value, bool readOnly);

class SimpleServer {
    private:
        const char* ssid;
        const char* password;

        StoredValue* values;
        int valueCount;

        void info (String text);
        void error(String text);
    public: 
        AsyncWebServer server = AsyncWebServer(80);

        SimpleServer(StoredValue values[], int valueCount);
        bool init(const char* ssid, const char* password);   
        String generatePage();

        StoredValue* getValuePointer(int index);
        int getValueCount();
};

#endif