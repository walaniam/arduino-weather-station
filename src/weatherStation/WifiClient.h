#ifndef WIFI_CLIENT_H
#define WIFI_CLIENT_H

#define MODE_WIFI_CLIENT_ONLY 0
#define MODE_WIFI_SERVER 1

#include <Arduino.h>
#include <string.h>
#include <SoftwareSerial.h>
#include "constants.h"
#include "secrets.h"

class WifiClient {
  private:
    SoftwareSerial *esp8266;
  public:
    WifiClient();
    ~WifiClient();
    void begin(SoftwareSerial *_esp8266, int mode);
    String atCommand(String command, const int timeout);
    void handleHttpRequest(String data);
    void sendPostRequest(String data);
    char myIp[16];
};

#endif
