// sensors.cpp
#include "sensors.h"
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include "config.h"

extern float tempOffset;
extern float humOffset;
extern int co2Offset;
    
SensirionI2cScd4x scd4x;
uint16_t co2 = 0;
float temperature = 0.0;
float humidity = 0.0;

void initSensors() {
  scd4x.begin(Wire, SCD41_ADDRESS);
  scd4x.stopPeriodicMeasurement();
  scd4x.startPeriodicMeasurement();
  // Включаем ASC
  scd4x.setAutomaticSelfCalibrationEnabled(true);
}

bool readSensors() {
  bool isDataReady = false;
  scd4x.getDataReadyStatus(isDataReady);
  if (isDataReady) {
    uint16_t error = scd4x.readMeasurement(co2, temperature, humidity);
    if (!error) {
      // Применяем коррекцию
      temperature += tempOffset;
      humidity += humOffset;
      co2 += co2Offset;

      // Ограничиваем влажность
      if (humidity < 0) humidity = 0;
      if (humidity > 100) humidity = 100;

      return true;
    }
  }
  return false;
}