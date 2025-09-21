// CO2_Monitor.ino
#include <Arduino.h>
#include <FS.h>
#include "config.h"
#include "wifi.h"
#include "sensors.h"
#include "display.h"
#include "ntp.h"
#include "web.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== CO2 Monitor Starting ===");

  initDisplay();
  initSensors();
  
  if (!SPIFFS.begin()) {
    Serial.println("❌ SPIFFS Mount Failed");
  } else {
    Serial.println("✅ SPIFFS OK");
    loadSettings();
  }
  
  if (connectWifi()) {
    if (syncNtpTime()) {
      Serial.println("✅ NTP Synced");
    }
  }
  startWebServer();
  Serial.println("✅ Web Server Started");
}

void loop() {
  if (readSensors()) {
    updateDisplay();
  }
  handleWebRequests();

  delay(5000);
}