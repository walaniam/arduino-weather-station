#include "Utils.h"

bool Utils::appendChar(char str[], char c, int bufferSize) {
  int len = strlen(str);
  if (len + 1 >= bufferSize) {
    Serial.print("Overflow, bufferSize=");
    Serial.println(bufferSize);
    return false;
  } else {
    str[len] = c;
    str[len + 1] = '\0';
    return true;
  }
}

String Utils::padded(int value) {
  String result = "";
  if (value < 10) {
    result.concat(F("0"));
  }
  result.concat(String(value));
  return result;
}

int Utils::freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int) __brkval);
}

void Utils::debug(String message) {
  if (WIFI_DEBUG) {
    int count = min(message.length(), 360);
    Serial.print(message.substring(0, count));
  }
}
