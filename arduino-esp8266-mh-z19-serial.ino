
#define INTERVAL 5000
#define WIFI_MAX_ATTEMPTS_INIT 3 //set to 0 for unlimited, do not use more then 65535
#define WIFI_MAX_ATTEMPTS_SEND 1 //set to 0 for unlimited, do not use more then 65535
#define MAX_DATA_ERRORS 15 //max of errors, reset after them
#define USE_GOOGLE_DNS true

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include "WiFiUtils.h"
#include "WiFiCreds.h"
#include "bme280.h"
#include "mh-z19.h"

long previousMillis = 0;
int errorCount = 0;
WiFiClient client;
WiFiUtils wifiUtils;

WiFiServer server(80);

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void setup() {
  Serial.begin(115200); // Init console
  Serial.println("Setup started");

  unsigned long previousMillis = millis();
  setupCo2();
  setupBme();

  WiFi.mode(WIFI_STA);//be only wifi client, not station

  Serial.println("Connecting...");

  // attempt to connect to Wifi network:
  unsigned int attempt = 0;
  while ( WiFi.status() != WL_CONNECTED) {
    if (WIFI_MAX_ATTEMPTS_INIT != 0 && attempt > WIFI_MAX_ATTEMPTS_INIT)
      break;
    if (attempt >= 65535)
      attempt = 0;
    attempt++;
    Serial.println("Attempt " + String(attempt));
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    WiFi.begin(ssid, pass);
    delay(10000);
  }

  if (USE_GOOGLE_DNS)
    wifiUtils.setGoogleDNS();
  // you're connected now, so print out the data:
  Serial.println("Connected to network");
  wifiUtils.printCurrentNet();
  wifiUtils.printWifiData();

  Serial.println("Waiting for sensors to init");

  while (millis() - previousMillis < 10000)
    delay(1000);
  Serial.println("Setup finished");
  server.begin();
  Serial.println("");
}

void handleClient(int ppm) {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();       
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html><body>");
          client.println("All fine! ---> ppm:" + String(ppm));
          client.println("</body></html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < INTERVAL)
    return;
  previousMillis = currentMillis;
  Serial.println("loop started");

  if (errorCount > MAX_DATA_ERRORS)
  {
    Serial.println("Too many errors, resetting");
    delay(2000);
    resetFunc();
  }
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
  Serial.println("ppm");
  bool dataError = false;

  handleClient(ppm);
  Serial.println("loop finished");
}
