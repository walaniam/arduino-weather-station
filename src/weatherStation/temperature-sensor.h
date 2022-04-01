#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Arduino.h>
#include <math.h>

const int TEMPERATURE_PIN = 0;
const int THERMISTOR_VALUE = 3975;

float temperature();

#endif
