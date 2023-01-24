#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "constants.h"

class Utils {
  public:
    static bool appendChar(char* str, char c, int bufferSize);
    static int freeRam();
};

#endif
