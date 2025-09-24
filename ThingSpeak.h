// thingspeak.h
#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include <Arduino.h>

extern bool useThingSpeak;
extern String thingSpeakApiKey;
extern unsigned long thingSpeakChannel;
extern bool thingSpeakStatus;

void initThingSpeak();
void sendToThingSpeak();

#endif