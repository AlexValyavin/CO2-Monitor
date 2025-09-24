// CO2_Monitor.ino
#include <Arduino.h>
#include <FS.h>
#include "config.h"
#include "wifi.h"
#include "sensors.h"
#include "display.h"
#include "ntp.h"
#include "web.h"

unsigned long lastSaveTime = 0;
const unsigned long saveInterval = 3600000; // 1 час в мс
const int maxRecords = 48; // Максимум 48 записей (2 дня)

void saveSensorData() {
  // Формируем новую запись
  time_t currentTime = time(nullptr);
  String newData = "{\"t\":\"" + String(ctime(&currentTime)).substring(0, 19) + "\",\"c\":" + String(co2) + ",\"tmp\":" + String(temperature, 1) + ",\"h\":" + String(humidity, 0) + "}";

  // Читаем существующий файл
  File file = SPIFFS.open("/data.json", "r");
  String existingData;
  if (file) {
    existingData = file.readString();
    file.close();
  }

  // Разбиваем на массив записей
  String records[maxRecords];
  int recordCount = 0;
  int start = 0;
  while (start < existingData.length() && recordCount < maxRecords) {
    int end = existingData.indexOf('\n', start);
    if (end == -1) end = existingData.length();
    records[recordCount++] = existingData.substring(start, end);
    start = end + 1;
  }

  // Если записей >= maxRecords, сдвигаем массив
  if (recordCount >= maxRecords) {
    for (int i = 0; i < recordCount - 1; i++) {
      records[i] = records[i + 1];
    }
    recordCount--;
  }

  // Добавляем новую запись
  records[recordCount] = newData;
  recordCount++;

  // Сохраняем обновлённый массив
  file = SPIFFS.open("/data.json", "w");
  if (file) {
    for (int i = 0; i < recordCount; i++) {
      if (records[i].length() > 0) {
        file.println(records[i]);
      }
    }
    file.close();
    Serial.println("Saved sensor data: " + newData);
  } else {
    Serial.println("Failed to save sensor data");
  }
}

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

  unsigned long currentTime = millis();
  if (currentTime - lastSaveTime >= saveInterval) {
    lastSaveTime = currentTime;
    saveSensorData();
  }

  delay(5000);
}