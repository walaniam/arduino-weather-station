//#define DEBUG
//#define SET_TIME
//#define USE_SD
#include "AnalogTemperatureSensor.h"
#include <Dps310.h>
#include "DS1307.h"
#include "rgb_lcd.h"
#include "WiFiEsp.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#ifdef USE_SD
#include <SD.h>
#endif
#include "constants.h"
#include "secrets.h"

struct TempAndPressure {
  float temp;
  float pressure;
};

unsigned long lastLoopTime;
unsigned long buttonPressTime;
bool buttonState = true;
AnalogTemperatureSensor analogTemp = AnalogTemperatureSensor(ANALOG_TEMPERATURE_PIN);
DS1307 clock;
rgb_lcd lcd;
Dps310 pressureSensor = Dps310();

#ifdef USE_SD
File dataFile;
#endif

SoftwareSerial Serial1(WIFI_RX, WIFI_TX);
int status = WL_IDLE_STATUS;
WiFiEspClient wifiClient;

void setup() {

  Serial.begin(9600);
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

  // Serial1
  Serial.println(F("Initializing Serial1"));
  Serial1.begin(115200);
  while (!Serial1);
  Serial.println(F("Initialized Serial1"));

  // WiFi
  WiFi.init(&Serial1);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    while (true);
  }

  while (status != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to WPA SSID: "));
    Serial.println(SECRET_SSID);
    status = WiFi.begin(SECRET_SSID, SECRET_PASS);
  }

  Serial.print(F("Connected to the network: "));
  Serial.println(SECRET_SSID);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
  

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

  unsigned long now = millis();

  if (buttonPressTime > 0 && now - 1000 > buttonPressTime && digitalRead(BUTTON_PIN) == HIGH) {
    buttonState = !buttonState;
    if (buttonState) {
      lcd.display();
    } else {
      lcd.noDisplay();
    }
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
  logToFile(csv);
}

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

void logToFile(String data) {

  // test wifi
  //  cmd_send(F("AT+CWLAP"));
  //  delay(1000);
  //  cmd_read();

  wifiClient.stop();

  char server[] = "192.168.0.192";
  if (wifiClient.connect(server, 7070)) {
    Serial.println(F("Connected to server"));
    // Make a HTTP request
    wifiClient.println(F("GET /swagger-ui/index.html HTTP/1.1"));
    wifiClient.println(F("Host: 192.168.0.192"));
    wifiClient.println(F("Connection: close"));
    wifiClient.println();
    Serial.println(F("Request sent"));
  }

  while (wifiClient.available()) {
    char c = wifiClient.read();
    Serial.write(c);
  }
  Serial.println();

#ifdef USE_SD
  dataFile.println(data);
  dataFile.flush();
#endif
}

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

void cmd_send(String cmd) {
  Serial.print("cmd: ");
  Serial.println(cmd);
  Serial1.println(cmd);
}

void cmd_read() {
  while (Serial1.available()) {
    Serial.write(Serial1.read());
  }
  Serial.println();
}
