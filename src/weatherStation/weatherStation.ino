#include "temperature-sensor.h"

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.print("Current temperature: ");
  float currentTemp = temperature();
  Serial.println(currentTemp);
  delay(2000);
}
