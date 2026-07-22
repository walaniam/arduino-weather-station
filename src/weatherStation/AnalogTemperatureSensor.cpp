#include "AnalogTemperatureSensor.h"

// B-constant for Grove Temperature Sensor V1.2/V1.3 (NCP18WF104F03RC thermistor)
#define THERMISTOR_VALUE 4275
// Fine-tune per individual sensor unit if needed
#define TEMP_ADJUST 0.0

AnalogTemperatureSensor::AnalogTemperatureSensor(int pin) {
  sensorPin = pin;
}

float AnalogTemperatureSensor::measure() {
  int rawMeasure = analogRead(sensorPin);
  if (rawMeasure <= 0 || rawMeasure >= 1023) {
    return NAN;
  }
  float resistance = (float)(1023 - rawMeasure) * 10000 / rawMeasure;
  float temp = 1 / (log(resistance / 10000) / THERMISTOR_VALUE + 1 / 298.15) - 273.15;
  float tempAdjusted = temp + 1 * TEMP_ADJUST;
  return tempAdjusted;
}
