// web.h
#ifndef WEB_H
#define WEB_H

#include <Arduino.h>

extern float tempOffset;
extern float humOffset;
extern int co2Offset;
extern String googleScriptUrl;
extern bool useGoogle;
extern bool useThingSpeak;
extern String thingSpeakApiKey;
extern int thingSpeakChannel;

void startWebServer();
void handleWebRequests();
void loadSettings();
void saveSettings();

#endif