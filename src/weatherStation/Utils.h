#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

class Utils {
  public:
    static bool appendChar(char str[], char c, int bufferSize);
    static String padded(int value);
    static int freeRam();
};

#endif
