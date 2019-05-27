#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <homie.hpp>
#include "config.h"

#define PIN D1
#define SERIAL true
#define HOMIE_SERIAL true
#define CLIENTID "ESP8266Client-"

void isrHandler();
boolean reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void heartBeatHandler();

WiFiClient espClient;
PubSubClient client(MQTT_IP, MQTT_PORT, callback, espClient);
Homie homieCTRL = Homie(&client);

uint8_t flag_motion = 0;
uint8_t flag_heartBeat = 0;
uint8_t oldState = 0;
unsigned long lastReconnectAttempt = 0;
string ClientID;
Ticker heartBeat;

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

        HomieDevice homieDevice = HomieDevice(DEVICE_NAME, "Bewegungsmelder", WiFi.localIP().toString().c_str(),
                                              WiFi.macAddress().c_str(), FW_NAME, FW_VERSION,
                                              "esp8266", "");
        HomieNode pirNode = HomieNode("pir-sensor", "Motion Sensor", "Generic PIR");
        HomieProperties motion = HomieProperties("motion", "Motion",
                                                 false, true, "60",
                                                 homie::boolean_t);
        pirNode.addProp(motion);
        homieDevice.addNode(pirNode);
        homieCTRL.setDevice(homieDevice);
        ClientID = string(CLIENTID) + DEVICE_NAME;
        attachInterrupt(digitalPinToInterrupt(PIN), isrHandler, CHANGE);
        heartBeat.attach(55.0, heartBeatHandler);
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
        if(flag_motion) {
                string payload;
                if(!oldState) {
                        if(SERIAL) Serial.println("motion detected!");
                        payload = "true";
                        oldState = 1;
                }else{
                        if(SERIAL) Serial.println("motion ended!");
                        payload = "false";
                        oldState = 0;
                }
                string topic = "homie/" + string(DEVICE_NAME) + "/pir-sensor/motion";
                client.publish(topic.c_str(),payload.c_str(),true);
                flag_motion = 0;
        }
        if(flag_heartBeat) {
                long time = millis() / 1000;
                string topic = "homie/" + string(DEVICE_NAME) + "/$stats/uptime";
                char payload[20];
                sprintf(payload, "%ld", time);
                client.publish(topic.c_str(), payload,true);
                topic = "homie/" + string(DEVICE_NAME) + "/$stats/interval";
                client.publish(topic.c_str(), "60",true);
                flag_heartBeat = 0;
        }
}

boolean reconnect() {
        // Loop until we're reconnected
        return homieCTRL.connect(ClientID.c_str(), MQTT_USR, MQTT_PW);
}

void isrHandler(){
        flag_motion = 1;
}

void heartBeatHandler(){
        flag_heartBeat = 1;
}

void callback(char *topic, byte *payload, unsigned int length){

}
