#include "AnalogTemperatureSensor.h"
#include <Wire.h>
#include <Dps310.h>
#include "DS1307.h"
#include "rgb_lcd.h"
#include <SPI.h>
#include <SD.h>

#define INTERVAL 10000
#define ANALOG_TEMPERATURE_PIN 0
const String DATA_FILE = "data20220403.txt";

struct TempAndPressure {
  float temp;
  float pressure;
};

AnalogTemperatureSensor analogTemp = AnalogTemperatureSensor(ANALOG_TEMPERATURE_PIN);
DS1307 clock;
rgb_lcd lcd;
Dps310 pressureSensor = Dps310();
File dataFile;

void setup() {
  
  Serial.begin(9600);
  while (!Serial);

  // Start clock
  clock.begin();
  clock.fillByYMD(2022, 4, 3);
  clock.fillByHMS(22, 17, 30);
  clock.fillDayOfWeek(SUN);
  clock.setTime();//write time to the RTC chip

  // start LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);

  // Open file
  if (!SD.begin(4)) {
    Serial.println(F("Card initialization failed"));
    lcd.print("No SD card");
    while (true);
  }

//  dataFile = SD.open(DATA_FILE);
//  if (dataFile) {
//    Serial.println(F("Reading file..."));
//    while (dataFile.available()) {
//      Serial.write(dataFile.read());
//    }
//    dataFile.close();
//    Serial.println(F("Done"));
//  } else {
//    Serial.println(F("Cannot open file"));
//  }

  dataFile = SD.open(DATA_FILE, FILE_WRITE);
//  logToFile(dataFile.name());

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
  }
}

void loop() {

  String time = getTime();

  // Analog temperature
  float temperature1 = analogTemp.measure();

  TempAndPressure tempAndPressure = digitalTempAndPressure();
  float temperature2 = tempAndPressure.temp;
  float avgPressure_hPa = tempAndPressure.pressure;
  
  float avgTemperature = (temperature1 + temperature2) / 2;

  // Serial Message
  Serial.print(time);
  Serial.print(" : ");
  Serial.print("temp1 = ");
  Serial.print(temperature1);
  Serial.print(", temp2 = ");
  Serial.print(temperature2);
  Serial.print(", pressure hPa = ");
  Serial.println(avgPressure_hPa);

  // LCD
  lcd.clear();
  // Temp line
  lcd.setCursor(0, 0);
  lcd.print("C");
  lcd.print((char)223);
  lcd.print(" :  ");
  lcd.print(String(avgTemperature, 2));
  // Pressure line
  lcd.setCursor(0, 1);
  String pressureMessage = "hPa: ";
  pressureMessage.concat(String(avgPressure_hPa, 2));
  lcd.print(pressureMessage);

  String csv = "";
  csv.concat(time);
  csv.concat(",");
  csv.concat(String(avgTemperature, 2));  
  csv.concat(",");
  csv.concat(String(avgPressure_hPa, 2));
  logToFile(csv);

  delay(INTERVAL);
}

TempAndPressure digitalTempAndPressure() {
  
  uint8_t pressureCount = 20;
  uint8_t temperatureCount = 20;
  float pressure[pressureCount];
  float temperature[temperatureCount];
  int16_t measureStatus = pressureSensor.getContResults(temperature, temperatureCount, pressure, pressureCount);

  float temp = 0;
  float avgPressure_hPa = -1;
  if (measureStatus == 0) {
    float tempSum = 0;
    for (int16_t i = 0; i < temperatureCount; i++) {
      tempSum += temperature[i];
    }
    temp = tempSum / temperatureCount;  
  
    float pressureSum = 0;
    for (int16_t i = 0; i < pressureCount; i++) {
      pressureSum += pressure[i];
    }
    avgPressure_hPa = pressureSum / pressureCount / 100;
  } else {
    Serial.print("Measure failed: ");
    Serial.println(measureStatus);
  }

  return (TempAndPressure){temp, avgPressure_hPa};
}

void logToFile(String data) {
  dataFile.println(data);
  dataFile.flush();
}

String getTime() {
  clock.getTime();
  String time = "";
  time.concat(clock.year + 2000);
  time.concat(padded(clock.month));
  time.concat(padded(clock.dayOfMonth));
  time.concat(" ");
  time.concat(padded(clock.hour));
  time.concat(padded(clock.minute));
  time.concat(padded(clock.second));
  return time;
}

String padded(int value) {
  String result = "";
  if (value < 10) {
    result.concat("0");
  }
  result.concat(String(value));
  return result;
}
