// ntp.cpp
#include "ntp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include "config.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER_IP, UTC_OFFSET_SECONDS, 60000);
bool timeSynced = false;

bool syncNtpTime() {
  timeClient.begin();
  int ntpTries = 0;
  while (ntpTries < 20) {
    if (timeClient.update()) {
      timeSynced = true;
      Serial.println("✅ Time synced: " + getFormattedTime());
      return true;
    }
    Serial.print("NTP sync attempt ");
    Serial.println(ntpTries + 1);
    delay(1000);
    ntpTries++;
  }
  Serial.println("❌ NTP sync failed after 20 attempts!");
  return false;
}

String getFormattedTime() {
  if (!timeSynced) return "N/A";
  time_t now = timeClient.getEpochTime();
  struct tm * timeinfo = localtime(&now);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

int getCurrentHour() {
  if (!timeSynced) return -1;
  time_t now = timeClient.getEpochTime();
  struct tm * timeinfo = localtime(&now);
  return timeinfo->tm_hour;
}