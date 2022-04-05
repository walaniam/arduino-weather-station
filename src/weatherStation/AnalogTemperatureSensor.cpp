#include "AnalogTemperatureSensor.h"

#define THERMISTOR_VALUE 3975

AnalogTemperatureSensor::AnalogTemperatureSensor(int pin) {
  sensorPin = pin;
}

float AnalogTemperatureSensor::measure() {
  int rawMeasure = analogRead(sensorPin);
  float resistance = (float)(1023 - rawMeasure) * 10000 / rawMeasure;
  float temp = 1 / (log(resistance / 10000) / THERMISTOR_VALUE + 1 / 298.15) - 273.15;
  return temp;
}
