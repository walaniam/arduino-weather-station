#include "temperature-sensor.h"
#include <Dps310.h>
#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;
Dps310 pressureSensor = Dps310();

void setup() {
  
  Serial.begin(9600);
  while (!Serial);

  // start LCD
  lcd.begin(16, 2);

  // start pressure
  pressureSensor.begin(Wire);
  
  int16_t temp_mr = 2;
  int16_t temp_osr = 2;
  int16_t prs_mr = 2;
  int16_t prs_osr = 2;
  int16_t ret = pressureSensor.startMeasureBothCont(temp_mr, temp_osr, prs_mr, prs_osr);
  
  if (ret != 0) {
    Serial.print("Init FAILED! ret = ");
    Serial.println(ret);
  } else {
    Serial.println("Init complete!");
  }
}

void loop() {

  float currentTemp = temperature();
  
  String message = "Celsius: ";
  message.concat(String(currentTemp, 2));
  
  Serial.println(message);

  lcd.clear();
  lcd.print(message);

  pressure();
  
  delay(10000);
}


void pressure() {

  uint8_t pressureCount = 20;
  float pressure[pressureCount];
  uint8_t temperatureCount = 20;
  float temperature[temperatureCount];

  int16_t ret = pressureSensor.getContResults(temperature, temperatureCount, pressure, pressureCount);

  
  if (ret != 0) {
    Serial.println();
    Serial.println();
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  } else {
    Serial.println();
    Serial.println();
    Serial.print(temperatureCount);
    Serial.println(" temperature values found: ");
    
    for (int16_t i = 0; i < temperatureCount; i++) {
      Serial.print(temperature[i]);
      Serial.println(" degrees of Celsius");
    }

    Serial.println();
    Serial.print(pressureCount);
    Serial.println(" pressure values found: ");
    for (int16_t i = 0; i < pressureCount; i++) {
      Serial.print(pressure[i]);
      Serial.println(" Pascal");
    }
  }
  
}
