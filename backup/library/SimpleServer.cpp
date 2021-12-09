#include "SimpleServer.h"

// --------------------- STORED VALUE ----------------------

StoredValue::StoredValue(String name, void* value, int type, bool readOnly) {
    if (name.length() > 16)
        Serial.println("[StoredValue][!] - Variable name \"" + name + "\" is longer then 16 characters!");
    
    for (int i = 0; i < name.length(); i++)
        this->name[i] = name[i];
    this->name[name.length()] = 0x00;

    this->value = value;
    this->type = type;
    this->readOnly = readOnly;
}

int StoredValue::getType() {
    return this->type;
}

bool StoredValue::isReadOnly() {
    return this->readOnly;
}

String StoredValue::getName() {
    return String(name);
}

StoredValue StoredInt(String name, void* value, bool readOnly) {
    return StoredValue(name, value, 0, readOnly);
}

StoredValue StoredFloat(String name, void* value, bool readOnly) {
    return StoredValue(name, value, 1, readOnly);
}

StoredValue StoredString(String name, void* value, bool readOnly) {
    return StoredValue(name, value, 2, readOnly);
}

// --------------------- SIMPLE SERVER ---------------------

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

#include "pages.h"

String SimpleServer::generatePage() {
    String generated = "";

    for (int i = 0; i < valueCount; i++) {
        String value = "";
        String type = "";

        switch (values[i].getType())
        {
        case 0:
            value = String(*(int*)(values[i].value));
            type = "number";
            break;
        case 1:
            value = String(*(float*)(values[i].value));
            type = "number";
            break;
        case 2:
            value = (*(String*)(values[i].value));
            type = "text";
        }

        generated += "<tr><td>" + values[i].getName() + "</td><td>" + ((values[i].isReadOnly()) ? value : "<input id=\"val" + String(i) + "\" type=\"" + type + "\" value=\"" + value + "\" step=\"" + ((values[i].getType() == 0) ? "1" : "0.01") + "\">") + "</td></tr>";
    }

    return INDEX_PAGE_1 + generated + INDEX_PAGE_2;
}

SimpleServer::SimpleServer(StoredValue values[], int valueCount) {
    this->values = values;
    this->valueCount = valueCount;
}

void SimpleServer::info(String text) {
    Serial.println(PREFIX + INFO + " - " + text);
}

void SimpleServer::error(String text) {
    Serial.println(PREFIX + ERROR + " - " + text);
}
 
StoredValue* SimpleServer::getValuePointer(int index) {
    return &values[index];
}

int SimpleServer::getValueCount() {
    return valueCount;
}

bool SimpleServer::init(const char* ssid, const char* password) {
    info("Initializing...");

    this->ssid     = ssid;
    this->password = password;

    WiFi.mode(WIFI_STA);
    WiFi.begin(this->ssid, this->password);
    WiFi.setSleep(false);

    info("Connecting to WiFi...");

    int tries = 1;
    Serial.print("Progress: [");
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
        delay(1000);
        Serial.print(".");
        tries++;
    }
    Serial.print("]\n");

    if (tries >= 20) {
        error("Failed to connect to WiFi!");
        return false;
    }

    info("IP Address: " + WiFi.localIP().toString());

    auto temp = this;
    SimpleServer* pntr = temp;
    server.on("/", HTTP_GET, [pntr](AsyncWebServerRequest *request){
        const String page = pntr->generatePage();
        request->send(200, "text/html", page);
    });
    server.on("/post", HTTP_POST, [pntr](AsyncWebServerRequest *request){
        for (int i = 0; i < pntr->getValueCount(); i++) {
            String name = "val" + String(i);
            String message;
            if (request->hasParam(name, true)) {
                message = request->getParam(name, true)->value();
                StoredValue* value = pntr->getValuePointer(i);
                switch (value->getType())
                {
                case 0:
                    (*(int*)(value->value)) = message.toInt();
                    Serial.println(value->getName() + ": " + (*(int*)(value->value)));
                    break;
                case 1:
                    (*(float*)(value->value)) = message.toFloat();
                    Serial.println(value->getName() + ": " + (*(float*)(value->value)));
                    break;
                case 2:
                    (*(String*)(value->value)) = message;
                    Serial.println(value->getName() + ": " + (*(String*)(value->value)));
                    break;
                }
            }
        }
        const String page = pntr->generatePage();
        request->send(200, "text/html", page);
    });
    server.onNotFound(notFound);

    this->server.begin();

    info("Variable count: " + String(valueCount));

    for (int i = 0; i < valueCount; i++)
    {
        switch (values[i].getType())
        {
        case 0:
            info("Integer - " + values[i].getName() + ": " + String(*(int*)(values[i].value)));
            break;
        case 1:
            info("Float - " + values[i].getName() + ": " + String(*(float*)(values[i].value)));
            break;
        case 2:        
            info("String - " + values[i].getName() + ": " + *(String*)(values[i].value));
            break;
        }   
    }

    info("Started succesfully!");
    return true;
}