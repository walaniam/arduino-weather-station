#ifndef WIFI_CLIENT_H
#define WIFI_CLIENT_H

#define MODE_WIFI_CLIENT_ONLY 0
#define MODE_WIFI_SERVER 1

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Utils.h"
#include "constants.h"
#include "secrets.h"

class WifiClient {
  private:
    SoftwareSerial *esp8266;
  public:
    WifiClient();
    ~WifiClient();
    void begin(SoftwareSerial *_esp8266, int mode);
    void atCommand(String command, const int timeout);
    String atCommandWithResponse(String command, const int timeout);
    void handleHttpRequest(char data[]);
    void sendPostRequest(char data[]);
    char myIp[16];
};

#endif
