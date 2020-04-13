
#define INTERVAL 6000
#define MAX_DATA_ERRORS 15 //max of errors, reset after them

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "FS.h"
#include "WiFiUtils.h"
#include "WiFiCreds.h"
#include "bme280.h"
#include "mh-z19.h"

long previousMillis = 0;
bool initial = false;
WiFiClient client;
WiFiUtils wifiUtils;

PubSubClient mqttClient(client); // Initialize the PuBSubClient library.
const char* mqtt_server = "192.168.0.94"; 
long channelID = 1027883;

String str = WiFi.macAddress();
int str_len = str.length() + 1; 
 
// Prepare the character array (the buffer) 
char clientId[28];
 
void setup() {
  // Copy it over 
  str.toCharArray(clientId,str_len);
  
  Serial.begin(9600); // Init console
  Serial.println("Setup started");

  if (SPIFFS.begin()) {
    Serial.println("Filesystem mounted");
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    Serial.print("Free bytes: ");
    Serial.println(fs_info.totalBytes - fs_info.usedBytes);
  } else {
    Serial.println("Failed to mount filesystem");
  }

  // Enabel Co2 Sensor
  pinMode(D5, OUTPUT);
  digitalWrite(D5, HIGH);

  unsigned long previousMillis = millis();
  setupCo2();
  setupBme();

  if (WiFi.SSID() == "") {
    initial = true;
  } else {
    WiFi.mode(WIFI_STA);//be only wifi client, not station
       
    // attempt to connect to Wifi network:
    int connRes = WiFi.waitForConnectResult();
    // you're connected now, so print out the data:
    Serial.println("Connected to network:"+ connRes);
  }

  if (WiFi.status()!=WL_CONNECTED){
    Serial.println("Failed to connect, finishing setup anyway");
  } else {
    // you're connected now, so print out the data:
    Serial.println("Connected to network:");
    wifiUtils.printCurrentNet();
    wifiUtils.printWifiData();
    WiFi.printDiag(Serial);
  }

  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  mqttClient.setServer(mqtt_server, 1883);   // Set the MQTT broker details.
  // Connect to the MQTT broker
  if (mqttClient.connect(clientId,mqttUserName,mqttPass)) 
  {
    Serial.println("connected");
  } else {
    Serial.print("failed, rc=");
    // Print to know why the connection failed.
    // See http://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
    Serial.print(mqttClient.state());
  }


  Serial.println("Setup finished");
 
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void loop()
{
  if (initial) {
    WiFiManager wifiManager;
    //starts an access point 
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.startConfigPortal("")) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
    ESP.reset();
  }
  ArduinoOTA.handle();

  // Connect to the MQTT broker if necessary
  if (!mqttClient.connected()) {
    if (mqttClient.connect(clientId,mqttUserName,mqttPass)) 
    {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      // Print to know why the connection failed.
      // See http://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
      Serial.print(mqttClient.state());
      delay(1000);
    }
  }

  mqttClient.loop();
  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < INTERVAL)
    return;
  previousMillis = currentMillis;
  Serial.println("loop started");

  Serial.println("reading data:");
  int ppm = readCo2();
 
  
  bmeData bmeData = readBme();

  Serial.print("Temp: ");
  Serial.print(bmeData.temp);
  Serial.print("°C");
  Serial.print("\tHumidity: ");
  Serial.print(bmeData.hum);
  Serial.print("% RH");
  Serial.print("\tPressure: ");
  Serial.print(bmeData.pres);
  Serial.print("Pa");
  Serial.print("\tCo2: ");
  Serial.print(ppm);
  Serial.println(" ppm");

  // Create data string to send to ThingSpeak
  String data = String("field1=" + String(bmeData.temp, DEC) + "&field2=" + String(bmeData.hum, DEC) + "&field3=" + String(ppm, DEC) + "&field4=" + String(bmeData.pres, DEC));
  int length = data.length();
  char msgBuffer[length];
  data.toCharArray(msgBuffer,length+1);
  
  // Create a topic string and publish data to ThingSpeak channel feed. 
  String topicString ="channels/" + String( channelID ) + "/publish/"+String(writeAPIKey);
  length=topicString.length();
  char topicBuffer[length];
  topicString.toCharArray(topicBuffer,length+1);
 
  if (mqttClient.publish( topicBuffer, msgBuffer )) {
    Serial.println("Data sent to MQTT");
  } else {
    Serial.println("Failed to send data"); 
  }
   
  //ESP.deepSleep(30e6); 
}
