#include "secrets.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define PUMP_PUB_TOPIC "sensor/esp32B/pumpPub"
#define AVAILABILITY_TOPIC "sensor/avaliability"
#define PUMP_SUB_TOPIC "sensor/esp32B/pumpSub"



WiFiClient wifiClient;
PubSubClient pbClient(wifiClient);

const int pumpPin = 19; 
int speed = 0; 
int speedAmount = 5; 

void wifiConnect(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < MAX_WIFI_RETRIES) {
    Serial.println("");
    delay(1000);
    Serial.println("Retry: ");
    for(int i = 0; i<=5; i++){
      Serial.print(i + " . ");
      delay(1000);      }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("**** Wi-Fi connected *****");
        Serial.print("Local IP Address: ");
        Serial.println(WiFi.localIP());
      } else {
      Serial.println("Failed to connect to Wi-Fi. Check credentials and try again.");
      while (1) delay(1); 
    }
  }

}

void mqttConnect(){

  while(!pbClient.connected()){
    Serial.println("Connecting with MQTT broker");

    if(pbClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASSWORD)){
      Serial.println("**********************");
      Serial.println("connected to MQTT Broker");
      delay(1000);

      pbClient.publish(AVAILABILITY_TOPIC, "online", true);
      pbClient.subscribe(PUMP_SUB_TOPIC);

    }else{ // Retry to MQTT connect 
      Serial.print("Failed to connect to MQTT broker");
      Serial.print(pbClient.state());
      Serial.print("...Retry ...");
      delay(5000);
    }    

  }

}

void pumpWater(){
  analogWrite(pumpPin, 255); // Pin and speed
  delay(10000);        // Ca 1.5 deciliter
  analogWrite(pumpPin, 0);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  Serial.print("Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  payload[length] = '\0';

  // Check if the payload is ON
  if (strcmp((char*)payload, "'PUMP_ON'") == 0) {
    // Call the function to turn on the pump 
    pumpWater();
    Serial.print("Pumping water....");
  }
}

void setup() { 
  Serial.begin(9600);
  pinMode(pumpPin, OUTPUT);
  wifiConnect();
  pbClient.setServer(MQTT_SERVER, MQTT_PORT);
  pbClient.setCallback(callback);
  mqttConnect();


	} 

void loop() { 

  pbClient.loop();

}

