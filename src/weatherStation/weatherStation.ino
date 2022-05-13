//#define DEBUG
//#define SET_TIME
//#define USE_SD
#include <SoftwareSerial.h>
#ifdef USE_SD
#include <SD.h>
#endif

#include "AnalogTemperatureSensor.h"
#include <Dps310.h>
#include "DS1307.h"
#include "rgb_lcd.h"
#include "constants.h"
#include "secrets.h"

struct TempAndPressure {
  float temp;
  float pressure;
};

AnalogTemperatureSensor analogTemp = AnalogTemperatureSensor(ANALOG_TEMPERATURE_PIN);
DS1307 clock;
rgb_lcd lcd;
Dps310 pressureSensor = Dps310();
SoftwareSerial esp8266(WIFI_RX, WIFI_TX);
#ifdef USE_SD
File dataFile;
#endif

unsigned long lastLoopTime;
bool buttonState = true;
String lastWeatherData = "no data";

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

  // Software Serial
  Serial.println(F("Initializing Serial1"));
  esp8266.begin(SERIAL_SPEED);
  while (!esp8266);
  Serial.println(F("Initialized Serial1"));

  // WiFi
  initWifiModule();

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
}


void loop() {

  handleHttpRequest();

  unsigned long now = millis();

  if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonState = !buttonState;
    if (buttonState) {
      lcd.display();
    } else {
      lcd.noDisplay();
    }
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

  String csv = "";
  csv.concat(time);
  csv.concat(F(","));
  csv.concat(String(temperature1, 2));
  csv.concat(F(","));
  csv.concat(String(temperature2, 2));
  csv.concat(F(","));
  csv.concat(String(avgPressure_hPa, 2));
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

void logData(String data) {
  lastWeatherData = data;
#ifdef USE_SD
  dataFile.println(data);
  dataFile.flush();
#endif
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

void handleHttpRequest() {
  if (esp8266.available()) {
    if (esp8266.find("+IPD,")) {
      delay(500);

      int connectionId = esp8266.read() - 48;

      // send headers
//      String headers = "HTTP/1.1 200 OK";
//      String headersSend = "AT+CIPSEND=";
//      headersSend += connectionId;
//      headersSend += ",";
//      headersSend += headers.length();
//      headersSend += "\r\n";
//      
//      atCommand(headersSend, 1000);
//      atCommand(headers, 1000);

      // send response body
      String webpage = lastWeatherData;
      String cipSend = "AT+CIPSEND=";
      cipSend += connectionId;
      cipSend += ",";
      cipSend += webpage.length();
      cipSend += "\r\n";
      atCommand(cipSend, 1000);
      atCommand(webpage, 1000);

      // close connection
      String closeCommand = "AT+CIPCLOSE=";
      closeCommand += connectionId; // append connection id
      closeCommand += "\r\n";
      atCommand(closeCommand, 3000);
    }
  }
}

void initWifiModule() {

  Serial.print(F("Connecting wifi to: "));
  Serial.println(SECRET_SSID);

  // reset module
  atCommand("AT+RST\r\n", 2000);

  String wifiSignInCommand = "AT+CWJAP=";
  wifiSignInCommand += "\"";
  wifiSignInCommand += SECRET_SSID;
  wifiSignInCommand += "\"";
  wifiSignInCommand += ",";
  wifiSignInCommand += "\"";
  wifiSignInCommand += SECRET_PASS;
  wifiSignInCommand += "\"";
  wifiSignInCommand += "\r\n";
  // sign in to wifi network
  atCommand(wifiSignInCommand, 2000);
  delay(3000);

  // set client mode
  atCommand("AT+CWMODE=1\r\n", 1500);
  delay(1500);

  // show assigned ip
  atCommand("AT+CIFSR\r\n", 1500);
  delay(1500);

  // set multiple connections
  atCommand("AT+CIPMUX=1\r\n", 1500);
  delay(1500);

  atCommand("AT+CIPSERVER=1,80\r\n", 1500);
  delay(1500);
}

String atCommand(String command, const int timeout) {

  esp8266.flush();
  esp8266.print(command);
  esp8266.flush();

  delay(100);

  String response = "";
  long int time = millis();
  while ( (time + timeout) > millis()) {
    while (esp8266.available()) {
      char c = esp8266.read();
      response += c;
    }
  }
  if (WIFI_DEBUG) {
    Serial.println(response);
  }
  return response;
}
