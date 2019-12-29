#include <Arduino.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <homie.hpp>
#include "config.h"

#define PIN D1
#define SERIAL false
#define HOMIE_SERIAL false
#define CLIENTID "ESP8266Client-"

void httpServer_ini();
void handleStatus();
void ICACHE_RAM_ATTR isrHandler();
boolean reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void ICACHE_RAM_ATTR heartBeatHandler();

ESP8266HTTPUpdateServer httpUpdater;
ESP8266WebServer httpServer(80);
WiFiClient espClient;
PubSubClient client(MQTT_IP, MQTT_PORT, callback, espClient);
Homie homieCTRL = Homie(&client);

const char *update_path = "/firmware";
const char *update_username = USERNAME;
const char *update_password = PASSWORD;
uint8_t flag_motion = 0;
uint8_t flag_heartBeat = 0;
uint8_t isOn = 0;
unsigned long lastReconnectAttempt = 0;
string ClientID;

void setup() {
        Serial.begin(115200);
        while (!Serial);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                if(SERIAL) if(SERIAL) Serial.print(".");
        }
        while (!Serial);
        if(SERIAL) if(SERIAL) Serial.println("");
        if(SERIAL) if(SERIAL) Serial.print("Connected, IP address: ");
        if(SERIAL) if(SERIAL) Serial.println(WiFi.localIP());
        if(SERIAL) if(SERIAL) Serial.println();
        pinMode(PIN, INPUT);

        HomieDevice homieDevice = HomieDevice(DEVICE_NAME, "Bewegungsmelder", "",
                                              CHIP_TYPE);
        HomieNode pirNode = HomieNode("pir-sensor", "Motion Sensor", "Generic PIR");
        HomieProperties motion = HomieProperties("motion", "Motion",
                                                 false, true, "60",
                                                 homie::boolean_t);
        pirNode.addProp(motion);
        homieDevice.addNode(pirNode);
        homieCTRL.setDevice(homieDevice);
        ClientID = string(CLIENTID) + DEVICE_NAME;
        httpServer_ini();
        attachInterrupt(digitalPinToInterrupt(PIN), isrHandler, RISING);
}


void loop(){
        if (!homieCTRL.connected()) {
                unsigned long now = millis();
                if (now - lastReconnectAttempt > 5000) {
                        lastReconnectAttempt = now;
                        // Attempt to reconnect
                        if (reconnect()) {
                                lastReconnectAttempt = 0;
                        }
                }
        }
        homieCTRL.loop();
        httpServer.handleClient();
        MDNS.update();
        if(flag_motion) {
                string payload;
                if(SERIAL) Serial.println("motion detected!");
                payload = "true";
                isOn = 1;
                string topic = "homie/" + string(DEVICE_NAME) + "/pir-sensor/motion";
                client.publish(topic.c_str(),payload.c_str(),true);
                flag_motion = 0;
        }
        if(isOn){
          int val = 1;
          val = digitalRead(PIN);
          if(!val){
              string payload;
              if(SERIAL) Serial.println("motion gone!");
              payload = "false";
              isOn = 0;
              string topic = "homie/" + string(DEVICE_NAME) + "/pir-sensor/motion";
              client.publish(topic.c_str(),payload.c_str(),true);
          }
        }
}

void httpServer_ini() {
        char buffer[100];
        sprintf(buffer, "%s", DEVICE_NAME);
        MDNS.begin(buffer);
        httpUpdater.setup(&httpServer, update_path, update_username, update_password);
        httpServer.on("/status",handleStatus);
        httpServer.begin();
        MDNS.addService("http", "tcp", 80);
        if(SERIAL) Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your "
                                 "browser and login with username '%s' and password '%s'\n",
                                 buffer, update_path, update_username, update_password);
        //------
}


boolean reconnect() {
        // Loop until we're reconnected
        return homieCTRL.connect(ClientID.c_str(), MQTT_USR, MQTT_PW);
}

void handleStatus() {
        String message;
        message += "name: " + String(DEVICE_NAME) + "\n";
        message += "chip: " + String(CHIP_TYPE) + "\n";
        message += "IP: " + WiFi.localIP().toString() + "\n";
        message +="free Heap: " + String(ESP.getFreeHeap()) + "\n";
        message += "heap Fragmentation: " + String(ESP.getHeapFragmentation()) + "\n";
        message += "MaxFreeBlockSize: " + String(ESP.getMaxFreeBlockSize()) + "\n";
        message += "ChipId: " + String(ESP.getChipId()) + "\n";
        message += "CoreVersion: " + String(ESP.getCoreVersion()) + "\n";
        message += "SdkVersion: " + String(ESP.getSdkVersion()) + "\n";
        message += "SketchSize: " + String(ESP.getSketchSize()) + "\n";
        message += "FreeSketchSpace: " + String(ESP.getFreeSketchSpace()) + "\n";
        message += "FlashChipId: " + String(ESP.getFlashChipId()) + "\n";
        message += "FlashChipSize: " + String(ESP.getFlashChipSize()) + "\n";
        message += "FlashChipRealSize: " + String(ESP.getFlashChipRealSize()) + "\n";
        httpServer.send(200, "text/plain", message);
}

void isrHandler(){
        flag_motion = 1;
}

void callback(char *topic, byte *payload, unsigned int length){

}
