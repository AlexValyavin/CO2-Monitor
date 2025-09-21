// sensors.h
#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

extern uint16_t co2;
extern float temperature;
extern float humidity;

void initSensors();
bool readSensors();

#endif