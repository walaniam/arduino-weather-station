//#define DEBUG
//#define SET_TIME
//#define USE_SD
#ifdef USE_SD
#include <SD.h>
#endif

#include <SoftwareSerial.h>
#include <Dps310.h>
#include "WifiClient.h"
#include "AnalogTemperatureSensor.h"
#include "DS1307.h"
#include "rgb_lcd.h"
#include "constants.h"

struct TempAndPressure {
  float temp;
  float pressure;
};

AnalogTemperatureSensor analogTemp(ANALOG_TEMPERATURE_PIN);
DS1307 clock;
rgb_lcd lcd;
Dps310 pressureSensor;
SoftwareSerial esp8266(WIFI_RX, WIFI_TX);
WifiClient wifiClient;
#ifdef USE_SD
File dataFile;
#endif

unsigned long lastLoopTime;
byte buttonState = 0;

char csvBuffer[CSV_BUFFER_SIZE] = "no data";

void setup() {

  Serial.begin(SERIAL_SPEED);
  while (!Serial);

  // Button
  pinMode(BUTTON_PIN, INPUT);

  // Clock
  clock.begin();
#ifdef SET_TIME
  clock.fillByYMD(2022, 4, 20);
  clock.fillByHMS(13, 38, 00);
  clock.fillDayOfWeek(WED);
  clock.setTime();
#endif

  // LCD
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);

  // WiFi
  Serial.println(F("Initializing esp8266 serial"));
  esp8266.begin(SERIAL_SPEED);
  while (!esp8266);
  Serial.println(F("Initialized esp8266 serial"));
  wifiClient.begin(&esp8266, MODE_WIFI_SERVER);

#ifdef DEBUG
  Serial.print(F("SRAM = "));
  Serial.println(freeRam());
#endif

  // Open file
#ifdef USE_SD
  if (!SD.begin(4)) {
    Serial.println(F("Card initialization failed"));
    lcd.print(F("No SD card"));
    while (true);
  }

#ifdef DEBUG
  dataFile = SD.open(DATA_FILE, FILE_READ);
  if (dataFile) {
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close();
    Serial.println(F("Done reading file"));
  } else {
    Serial.print(F("Cannot open file: "));
    Serial.println(DATA_FILE);
  }
#endif

  dataFile = SD.open(DATA_FILE, FILE_WRITE);
#endif

  // start Dps310
  pressureSensor.begin(Wire);

  int16_t temp_mr = 2;
  int16_t temp_osr = 2;
  int16_t prs_mr = 2;
  int16_t prs_osr = 2;
  int16_t measureStatus = pressureSensor.startMeasureBothCont(temp_mr, temp_osr, prs_mr, prs_osr);

  if (measureStatus != 0) {
    Serial.print(F("Init FAILED! measureStatus = "));
    Serial.println(measureStatus);
  }

#ifdef DEBUG
  Serial.print(F("SRAM = "));
  Serial.println(freeRam());
#endif

  Serial.println(F("setup done"));
}


void loop() {

  wifiClient.handleHttpRequest(csvBuffer);

  unsigned long now = millis();

  if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonState += 1;
    if (buttonState > 1) {
      buttonState = 0;
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Waiting..."));
    delay(1000);
  }

  if (now - lastLoopTime < INTERVAL) {
    return;
  }

  lastLoopTime = now;

  String time = getTime();

  // Analog temperature
  float temperature1 = analogTemp.measure();

  TempAndPressure tempAndPressure = digitalTempAndPressure();
  float temperature2 = tempAndPressure.temp;
  float avgPressure_hPa = tempAndPressure.pressure;

  // Serial Message
  Serial.print(time);
  Serial.print(F(" : "));
  Serial.print(F("temp1 = "));
  Serial.print(temperature1);
  Serial.print(F(", temp2 = "));
  Serial.print(temperature2);
  Serial.print(F(", pressure hPa = "));
  Serial.println(avgPressure_hPa);

  // LCD
  lcd.clear();
  if (buttonState == 0) {
    // Temp line
    lcd.setCursor(0, 0);
    lcd.print(F("C"));
    lcd.print((char)223);
    lcd.print(F(":"));
    lcd.print(String(temperature1, 2));
    lcd.print(F(" / "));
    lcd.print(String(temperature2, 2));
    // Pressure line
    lcd.setCursor(0, 1);
    String pressureMessage = "hPa: ";
    pressureMessage.concat(String(avgPressure_hPa, 2));
    lcd.print(pressureMessage);
  } else if (buttonState == 1) {
    lcd.setCursor(0, 0);
    lcd.print(F("IP:"));
    lcd.setCursor(0, 1);
    lcd.print(wifiClient.myIp);
  }

  char csv[CSV_BUFFER_SIZE];
  strcpy(csv, time.c_str());
  appendChar(csv, ',', CSV_BUFFER_SIZE);
  strcat(csv, String(temperature1, 2).c_str());
  appendChar(csv, ',', CSV_BUFFER_SIZE);
  strcat(csv, String(temperature2, 2).c_str());
  appendChar(csv, ',', CSV_BUFFER_SIZE);
  strcat(csv, String(avgPressure_hPa, 2).c_str());
  logData(csv);
}

/**
   Measure temperature and pressure.
*/
TempAndPressure digitalTempAndPressure() {

  uint8_t pressureCount = 20;
  uint8_t tempCount = 20;
  float pressure[pressureCount];
  float temperature[tempCount];
  int16_t measureStatus = pressureSensor.getContResults(temperature, tempCount, pressure, pressureCount);

  float temp = 0;
  float avgPressure_hPa = -1;
  if (measureStatus == 0) {
    float tempSum = 0;
    for (uint8_t i = 0; i < tempCount; i++) {
      tempSum += temperature[i];
    }
    temp = tempSum / tempCount;

    float pressureSum = 0;
    for (uint8_t i = 0; i < pressureCount; i++) {
      pressureSum += pressure[i];
    }
    avgPressure_hPa = pressureSum / pressureCount / 100;
  } else {
    Serial.print(F("Measure failed: "));
    Serial.println(measureStatus);
  }

  return (TempAndPressure) {
    temp, avgPressure_hPa
  };
}

void logData(char csvData[]) {
  strcpy(csvBuffer, csvData);
#ifdef USE_SD
  dataFile.println(data);
  dataFile.flush();
#endif
  wifiClient.sendPostRequest(csvBuffer);
}

/**
   Get formatter date-time.
*/
String getTime() {
  clock.getTime();
  String time = "";
  time.concat(clock.year + 2000);
  time.concat(padded(clock.month));
  time.concat(padded(clock.dayOfMonth));
  time.concat(F(" "));
  time.concat(padded(clock.hour));
  time.concat(padded(clock.minute));
  time.concat(padded(clock.second));
  return time;
}


bool appendChar(char* str, char c, int bufferSize) {
  int len = strlen(str);
  if (len + 1 >= bufferSize) {
    return false;
  } else {
    str[len] = c;
    str[len + 1] = '\0';
    return true;
  }
}

/**
   Prefix int value with zero-es
*/
String padded(int value) {
  String result = "";
  if (value < 10) {
    result.concat(F("0"));
  }
  result.concat(String(value));
  return result;
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int) __brkval);
}
