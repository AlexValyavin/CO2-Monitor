// wifi.cpp
#include "wifi.h"
#include <ESP8266WiFi.h>
#include "config.h"

bool connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to Wi-Fi");
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 30) {
    delay(1000);
    Serial.print(".");
    counter++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected: " + WiFi.localIP().toString());
    return true;
  } else {
    Serial.println("\nWi-Fi failed!");
    return false;
  }
}