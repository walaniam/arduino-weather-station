#include "temperature-sensor.h"
#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

void setup() {
  lcd.begin(16, 2);
  Serial.begin(9600);
}

void loop() {

  float currentTemp = temperature();
  
  String message = "Celsius: ";
  message.concat(String(currentTemp, 2));
  
  Serial.println(message);

  lcd.clear();
  lcd.print(message);
  
  delay(2000);
}
