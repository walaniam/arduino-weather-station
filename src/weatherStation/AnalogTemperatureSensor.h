#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Arduino.h>
#include <math.h>

class AnalogTemperatureSensor {
  public:
    int sensorPin;
    AnalogTemperatureSensor(int pin);
    float measure();
};

#endif
