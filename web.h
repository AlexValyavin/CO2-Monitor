// web.h
#ifndef WEB_H
#define WEB_H

#include <ESP8266WebServer.h>

extern float tempOffset;
extern float humOffset;
extern int co2Offset;
extern bool useThingSpeak;
extern String thingSpeakApiKey;
extern unsigned long thingSpeakChannel;

void startWebServer();
void handleWebRequests();
void loadSettings();
void saveSettings();

#endif