#include "Utils.h"

bool Utils::appendChar(char* str, char c, int bufferSize) {
  int len = strlen(str);
  if (len + 1 >= bufferSize) {
    Serial.print(F("Overflow, bufferSize="));
    Serial.println(bufferSize);
    return false;
  } else {
    str[len] = c;
    str[len + 1] = '\0';
    return true;
  }
}

int Utils::freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int) __brkval);
}
