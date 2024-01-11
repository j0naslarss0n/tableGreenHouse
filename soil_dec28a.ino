#include "secrets.h"
#include <ArduinoJson.h>
#include "Adafruit_seesaw.h"
#include <WiFi.h>
#include <PubSubClient.h>



#define CAPA_SOIL_TOPIC "sensor/" + String(DEVICE_NAME) + "/capcitance"
#define TEMP_SOIL_TOPIC "sensor/" + String(DEVICE_NAME) +"/temperature"
#define AVAILABILITY_TOPIC "sensor/avaliability"
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10800   /* Time ESP32 will go to sleep (in seconds)  : 3 H*/


Adafruit_seesaw seeSaw;
WiFiClient wifiClient;
PubSubClient pbClient(wifiClient);

void wifiConnect(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < MAX_WIFI_RETRIES) {
    
    delay(1000);
    Serial.print("Retry: ");
    Serial.println(retries);
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("**** Wi-Fi connected *****");
        Serial.print("Local IP Address: ");
        Serial.println(WiFi.localIP());
      } else {
      Serial.println("Failed to connect to Wi-Fi. Check credentials and try again.");
      while (1) delay(1); 
    }
  retries ++;
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

    }else{ // Retry to MQTT connect 
      Serial.print("Failed to connect to MQTT broker");
      Serial.print(pbClient.state());
      Serial.print("Retry ...");
      delay(5000);
    }    

  }

}
// Setup sensor to detect moisture and temperature
// PINS; RED;3.3V, BLACK;GND, GREEN;GPIO22, WHITE;GPIO21
void setupSensor(){

  if (!seeSaw.begin(0x36)) {
    Serial.println("ERROR! Seesaw not found");
    while (1) delay(1);
  } else {
    Serial.print("Seesaw started! Version: ");
    Serial.println(seeSaw.getVersion(), HEX);
  }

}


void publishMessage(){
  const int READINGS = 5;
  const int DRYVALUE = 350; // Min value
  const int WETVALUE = 650; // Max value

  float tempC = seeSaw.getTemp();
  int capacAvg[READINGS];
  int capacSum = 0;
  // Calculate average of several readings for more accurate result
  for (int i = 0; i < READINGS; ++i ){
     capacAvg[i] = seeSaw.touchRead(0);
     capacSum += capacAvg[i];
     delay(1000);
     Serial.println(capacAvg[i]);

  }
  int capacRead = capacSum / READINGS;
  
    // Create JSON document
  DynamicJsonDocument doc(512);
  doc["temperature"] = tempC;
  doc["capacitance"] = capacRead;

  // Serialize JSON to a String
  String payload;
  serializeJson(doc, payload);

    // Calculate moisture percentage
  float moisturePercentage = map(capacRead, DRYVALUE, WETVALUE, 0, 100);
  moisturePercentage = constrain(moisturePercentage, 0, 100);

  Serial.println("Capacitence: " + String(capacRead) + " value");
  delay(1000);

  if (pbClient.publish(String(CAPA_SOIL_TOPIC).c_str(),String(moisturePercentage).c_str(), true)) {
    Serial.println("Humidity published successfully, with topic;");
    Serial.println(String(CAPA_SOIL_TOPIC).c_str());
  } else {
    Serial.println("Failed to publish humidity");

  }

}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // Delay to avoid deep-sleep error
  delay(500);
  // Connect to Mosquitto
  pbClient.setServer(MQTT_SERVER, MQTT_PORT);
  Serial.println("Server set");
  setupSensor();
  Serial.println("Sensor set");
  wifiConnect();
  mqttConnect();
  if(!pbClient.connected()){
    mqttConnect();
  }
  publishMessage();
  pbClient.disconnect();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();

}

void loop() {
 // No loop with deep sleep

}
