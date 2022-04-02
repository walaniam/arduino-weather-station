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

  // start Dps310
  pressureSensor.begin(Wire);
  
  int16_t temp_mr = 2;
  int16_t temp_osr = 2;
  int16_t prs_mr = 2;
  int16_t prs_osr = 2;
  int16_t measureStatus = pressureSensor.startMeasureBothCont(temp_mr, temp_osr, prs_mr, prs_osr);
  
  if (measureStatus != 0) {
    Serial.print("Init FAILED! measureStatus = ");
    Serial.println(measureStatus);
  } else {
    Serial.println("Init complete!");
  }
}

void loop() {

  // Measure pressure and temperature
  uint8_t pressureCount = 20;
  uint8_t temperatureCount = 20;
  float pressure[pressureCount];
  float temperature[temperatureCount];
  int16_t measureStatus = pressureSensor.getContResults(temperature, temperatureCount, pressure, pressureCount);

  float temperature2 = 0;
  float avgPressure_hPa = -1;
  if (measureStatus == 0) {
    float tempSum = 0;
    for (int16_t i = 0; i < temperatureCount; i++) {
      tempSum += temperature[i];
    }
    temperature2 = tempSum / temperatureCount;  
  
    float pressureSum = 0;
    for (int16_t i = 0; i < pressureCount; i++) {
      pressureSum += pressure[i];
    }
    avgPressure_hPa = pressureSum / pressureCount / 100;
  } else {
    Serial.print("Measure failed: ");
    Serial.println(measureStatus);
  }

  // Analog temperature
  float temperature1 = analogTemperature();
  float avgTemperature = (temperature1 + temperature2) / 2;

  // Serial Message
  Serial.print("temp1 = ");
  Serial.print(temperature1);
  Serial.print(", temp2 = ");
  Serial.print(temperature2);
  Serial.print(", pressure hPa = ");
  Serial.println(avgPressure_hPa);

  // LCD
  String pressureMessage = "hPa: ";
  pressureMessage.concat(String(avgPressure_hPa, 2));

  lcd.clear();
  // Temp line
  lcd.setCursor(0, 0);
  lcd.print("C");
  lcd.print((char)223);
  lcd.print(" :  ");
  lcd.print(String(avgTemperature, 2));
  // Pressure line
  lcd.setCursor(0, 1);
  lcd.print(pressureMessage);

  delay(30000);
}
