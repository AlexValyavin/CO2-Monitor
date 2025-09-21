// google.cpp
#include "google.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "config.h"
#include "ntp.h"
#include "sensors.h"
#include "web.h"  // 👈 ДОБАВИТЬ

// Глобальные переменные (уже объявлены в web.h как extern)
extern String googleScriptUrl;  // 👈 УБЕДИТЬСЯ, ЧТО ЕСТЬ
extern uint16_t co2;
extern float temperature;
extern float humidity;

void sendDataToGoogle() {
  // Не отправляем, если URL пустой
  if (googleScriptUrl.length() < 10) {
    Serial.println("Google Script URL not set");
    return;
  }

  HTTPClient http;
  WiFiClientSecure client;

  String url = googleScriptUrl; // Используем глобальную переменную
  String payload = "{\"timestamp\":\"" + getFormattedTime() + "\"," +
                   "\"co2\":" + String(co2) + "," +
                   "\"temp\":" + String(temperature, 1) + "," +
                   "\"hum\":" + String(humidity, 0) + "}";

  client.setInsecure();
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.println("✅ Data sent to Google Sheets");
  } else {
    Serial.print("❌ Error sending to Google Sheets: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}