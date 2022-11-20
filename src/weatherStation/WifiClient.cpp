#include "WifiClient.h"
#include "Utils.h"

WifiClient::WifiClient() {
  esp8266 = NULL;
}

WifiClient::~WifiClient() {
}

void WifiClient::begin(SoftwareSerial *_esp8266) {

  esp8266 = _esp8266;

  Serial.print(F("Connecting wifi to: "));
  Serial.println(SECRET_SSID);

  // reset module
  WifiClient::atCommand(F("AT+RST\r\n"), 2000);

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
  WifiClient::atCommand(wifiSignInCommand, 2000);
  delay(3000);

  // set client mode
  WifiClient::atCommand(F("AT+CWMODE=1\r\n"), 1500);
  delay(1500);

  // show assigned ip
  String ipResponse = WifiClient::atCommandWithResponse(F("AT+CIFSR\r\n"), 1500);
  int ipBegin = ipResponse.indexOf('"');
  ipResponse = ipResponse.substring(ipBegin + 1, ipResponse.length());
  int ipEnd = ipResponse.indexOf('"');
  ipResponse.replace('/', '.'); // sometimes / instead of . - don't know why :/
  ipResponse.toCharArray(myIp, ipEnd + 1);
  delay(1500);

#ifdef MODE_WIFI_SERVER  
  // set multiple connections
  WifiClient::atCommand(F("AT+CIPMUX=1\r\n"), 1500);
  delay(1500);
  // Start in server mode  
  WifiClient::atCommand(F("AT+CIPSERVER=1,80\r\n"), 1500);
  delay(1500);
#endif
}

void WifiClient::sendPostRequest(char data[]) {

//  ping gateway
//  WifiClient::atCommand("AT+PING=\"192.168.0.1\"\r\n", 3000);
//  delay(3000);
  
  // CIPSTART
  Serial.println(AZ_CONNECT);
  WifiClient::atCommand(AZ_CONNECT, 5000);
  delay(1000);
//  memset(atCommand, '\0', AT_COMMAND_BUFFER_SIZE);
  
  // Build POST request
  int dataLength = strlen(data);
//  char httpPayload[AT_COMMAND_BUFFER_SIZE + dataLength];
//  strcat(httpPayload, AZ_RAW_REQUEST);
//  strcat(httpPayload, String(dataLength).c_str());
//  strcat(httpPayload, "\r\n\r\n");
//  strcat(httpPayload, data);
  String httpPayload = AZ_RAW_REQUEST;
  httpPayload += dataLength;
  httpPayload += "\r\n\r\n";
  httpPayload += data;
  httpPayload += "\r\n";
  

  // CIPSEND
  String cipSend = "AT+CIPSEND=";
  cipSend += httpPayload.length(); // strlen(httpPayload);
  cipSend += "\r\n";
  WifiClient::atCommand(cipSend, 1000);
  // POST body send
  Serial.println(httpPayload);
  WifiClient::atCommand(httpPayload, 1000);
  
//  WifiClient::atCommand("AT+CIPSTATUS\r\n", 5000);
//  delay(1000);

  // close connection
  WifiClient::atCommand(F("AT+CIPCLOSE\r\n"), 3000);
  delay(1000);
}

void WifiClient::handleHttpRequest(char responseBody[]) {
  if (esp8266->available()) {
    if (esp8266->find("+IPD,")) {
      delay(500);

      int connectionId = esp8266->read() - 48;

      int bodyLength = strlen(responseBody);
      char httpPayload[bodyLength + 120] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: ";
      strcat(httpPayload, String(bodyLength).c_str());
      strcat(httpPayload, "\r\n\r\n");
      strcat(httpPayload, responseBody);

      // send response
      String cipSend = "AT+CIPSEND=";
      cipSend += connectionId;
      cipSend += ",";
      cipSend += strlen(httpPayload);
      cipSend += "\r\n";
      WifiClient::atCommand(cipSend, 1000);
      WifiClient::atCommand(httpPayload, 1000);

      // close connection
      String closeCommand = "AT+CIPCLOSE=";
      closeCommand += connectionId;
      closeCommand += "\r\n";
      WifiClient::atCommand(closeCommand, 3000);
    }
  }
}

void WifiClient::atCommand(String command, const int timeout) {

  if (WIFI_DEBUG) {
    Serial.println(F("---BEGIN---"));
    int count = min(command.length(), 15);
    Serial.print(command.substring(0, count));
    Serial.println(F("..."));
  }

  esp8266->flush();
  esp8266->print(command);
  esp8266->flush();

  delay(100);

  long int time = millis();
  while ((time + timeout) > millis()) {
    while (esp8266->available()) {
      char c = esp8266->read();
      if (WIFI_DEBUG) {
        Serial.print(c);
      }
    }
  }
  if (WIFI_DEBUG) {
    Serial.println(F("---END---"));
  }
}

String WifiClient::atCommandWithResponse(String command, const int timeout) {

  if (WIFI_DEBUG) {
    Serial.println(F("---BEGIN---"));
    int count = min(command.length(), 25);
    Serial.print(command.substring(0, count));
    Serial.println(F("..."));
  }

  esp8266->flush();
  esp8266->print(command);
  esp8266->flush();

  delay(100);

  String response;
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (esp8266->available()) {
      char c = esp8266->read();
      if (WIFI_DEBUG) {
        Serial.print(c);
      }
      response += c;
    }
  }
  if (WIFI_DEBUG) {
    Serial.println(F("---END---"));
  }
  return response;
}
