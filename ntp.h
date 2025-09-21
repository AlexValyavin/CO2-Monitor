// ntp.h
#ifndef NTP_H
#define NTP_H

#include <Arduino.h>

extern bool timeSynced;

bool syncNtpTime();
String getFormattedTime();
int getCurrentHour();

#endif