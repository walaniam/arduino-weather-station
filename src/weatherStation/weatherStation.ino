#include "constants.h"
#include "secrets.h"
#include <SoftwareSerial.h>
#include <Dps310.h>
#include "Utils.h"
#include <WiFiEsp.h>
#include "AnalogTemperatureSensor.h"
#include "DS1307.h"
#include "rgb_lcd.h"

struct TempAndPressure {
  float temp;
  float pressure;
};

AnalogTemperatureSensor analogTemp(ANALOG_TEMPERATURE_PIN);
DS1307 clock;
rgb_lcd lcd;
Dps310 pressureSensor;
SoftwareSerial Serial1(WIFI_RX, WIFI_TX);
WiFiEspClient client;
int status = WL_IDLE_STATUS;     // the Wifi radio's status

char csv[CSV_BUFFER_SIZE];
unsigned long lastLoopTime = 0;
byte buttonState = 0;

void setup() {

  Serial.begin(115200);
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

  // ESP8266
  Serial.println(F("Initializing esp8266, Serial1"));
  changeBaudRate();
  Serial.println(F("Initialized esp8266 serial"));

  // WiFi
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to WPA SSID: "));
    Serial.println(SECRET_SSID);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(SECRET_SSID, SECRET_PASS);
  }

  // you're connected now, so print out the data
  Serial.println(F("You're connected to the network"));
  printWifiStatus();

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

#ifdef DEBUG_RAM
  Serial.print(F("SRAM = "));
  Serial.println(Utils::freeRam());
#endif

  Serial.println(F("setup done"));
}

void changeBaudRate() {

  Serial1.begin(115200);

  Serial.println(F("Sending baud rate change..."));
  Serial1.print(F("AT+UART_DEF="));
  Serial1.print(9600);
  Serial1.println(F(",8,1,0,0"));
  delay(500);
  // we can't expect a readable answer over SoftwareSerial at 115200

  Serial1.begin(9600);
  while (!Serial1);

}

void loop() {

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

  if (lastLoopTime != 0 && now - lastLoopTime < INTERVAL) {
    return;
  }

  memset(csv, 0, sizeof(csv));
  lastLoopTime = now;
  readTime();

  // Analog temperature
  float temperature1 = analogTemp.measure();

  TempAndPressure tempAndPressure = digitalTempAndPressure();
  float temperature2 = tempAndPressure.temp;
  float avgPressure_hPa = tempAndPressure.pressure;

  // Serial Message
  printIsoDate();
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
    lcd.print(F("hPa: "));
    lcd.print(String(avgPressure_hPa, 2));
  } else if (buttonState == 1) {
    lcd.setCursor(0, 0);
    lcd.print(F("IP:"));
    lcd.setCursor(0, 1);
    // TODO
    //    lcd.print(wifiClient.myIp);
  }

  Utils::appendChar(csv, ',', CSV_BUFFER_SIZE);
  strcat(csv, String(temperature1, 2).c_str());
  Utils::appendChar(csv, ',', CSV_BUFFER_SIZE);
  strcat(csv, String(temperature2, 2).c_str());
  Utils::appendChar(csv, ',', CSV_BUFFER_SIZE);
  strcat(csv, String(avgPressure_hPa, 2).c_str());

  sendWeatherReport(csv);
}

void printIsoDate() {
  for (uint8_t i = 0; i < 16; i++) {
    Serial.print(csv[i]);
  }
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

void sendWeatherReport(char data[]) {

  client.stop();

  // if there's a successful connection
  if (client.connect(SRV_CONNECT_HOST, SRV_CONNECT_PORT)) {

    Serial.println(F("Sending request..."));

    // http method
    client.print(F("POST "));
    client.print(SRV_URI);
    client.println(F(" HTTP/1.1"));

    // headers
    client.print(F("Host: "));
    client.println(SRV_REQ_HOST);

    client.println(F("User-Agent: Arduino"));
    client.println(F("Accept: */*"));
    client.println(F("Content-Type: application/x-www-form-urlencoded"));
    client.println(F("Connection: close"));

    int dataLength = strlen(data);
    char lengthHeader[32];
    sprintf(lengthHeader, "Content-Length: %u", dataLength);
    client.println(lengthHeader);

    // Request payload
    client.println();
    client.println(data);

    client.flush();

    Serial.println(F("Request sent"));

    delay(3000);
    // Read if anything available
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
    Serial.println();

    client.stop();

  } else {
    // if you couldn't make a connection
    Serial.println(F("Connection failed"));
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print(F("Signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));
}

void readTime() {
  clock.getTime();
  uint16_t year = clock.year + 2000;
  sprintf(csv, "%u%02u%02u %02u%02u%02u", year, clock.month, clock.dayOfMonth, clock.hour, clock.minute, clock.second);
}
