#include "temperature-sensor.h"

float temperature() {
  int rawMeasure = analogRead(TEMPERATURE_PIN);
  float resistance = (float)(1023 - rawMeasure) * 10000 / rawMeasure;
  float temp = 1 / (log(resistance/10000) / THERMISTOR_VALUE + 1 / 298.15) - 273.15;
  return temp;
}
