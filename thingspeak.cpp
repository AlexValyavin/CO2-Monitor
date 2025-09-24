// thingspeak.cpp
#include "thingspeak.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "web.h"
#include "sensors.h"
#include "config.h"

WiFiClient client;
unsigned long lastThingSpeakUpdate = 0; // Время последней отправки
const unsigned long thingSpeakInterval = 600000; // 10 минут в миллисекундах

void initThingSpeak() {
  if (useThingSpeak && thingSpeakApiKey != "" && thingSpeakChannel != 0) {
    Serial.println("✅ ThingSpeak initialized");
    thingSpeakStatus = false; // Начальный статус: не синхронизировано
  } else {
    Serial.println("❌ ThingSpeak not initialized: invalid settings");
    thingSpeakStatus = false;
  }
}

void sendToThingSpeak() {
  if (!useThingSpeak || thingSpeakApiKey == "" || thingSpeakChannel == 0) {
    thingSpeakStatus = false;
    return;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastThingSpeakUpdate < thingSpeakInterval && lastThingSpeakUpdate != 0) {
    return; // Не прошло 10 минут, пропускаем
  }

  // Проверка на nan и нулевые значения
  if (co2 == 0 || isnan(temperature) || isnan(humidity) || temperature == 0 || humidity == 0) {
    Serial.println("❌ Invalid sensor data (CO2=" + String(co2) + ", Temp=" + String(temperature, 1) + ", Hum=" + String(humidity, 1) + "), skipping ThingSpeak update");
    thingSpeakStatus = false;
    return;
  }

  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=" + thingSpeakApiKey;
  url += "&field1=" + String(co2);
  url += "&field2=" + String(temperature, 1);
  url += "&field3=" + String(humidity, 0);

  http.setTimeout(5000); // Таймаут 5 секунд
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("✅ Data sent to ThingSpeak: CO2=" + String(co2) + ", Temp=" + String(temperature, 1) + ", Hum=" + String(humidity, 0));
      lastThingSpeakUpdate = currentTime; // Обновляем время последней отправки
      thingSpeakStatus = true; // Успешная синхронизация
    } else {
      Serial.println("❌ Error sending data to ThingSpeak: HTTP code " + String(httpCode));
      thingSpeakStatus = false; // Ошибка синхронизации
    }
  } else {
    Serial.println("❌ Error sending data to ThingSpeak: connection failed");
    thingSpeakStatus = false; // Ошибка соединения
  }
  http.end();
}