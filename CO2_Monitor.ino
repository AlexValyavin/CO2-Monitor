// CO2_Monitor.ino
#include <Arduino.h>
#include <FS.h>
#include "config.h"
#include "wifi.h"
#include "sensors.h"
#include "display.h"
#include "ntp.h"
#include "web.h"
#include "thingspeak.h" // Добавлено

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 15000; // 15 сек для ThingSpeak

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
  initThingSpeak(); // Добавлено
  Serial.println("✅ Web Server Started");
}

void loop() {
  if (readSensors()) {
    updateDisplay();
  }
  handleWebRequests();

  unsigned long currentTime = millis();
  if (currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;
    sendToThingSpeak(); // Добавлено
  }

  delay(5000);
}