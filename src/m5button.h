#pragma once

#include <Arduino.h>

class m5button {
   public:
    static void Setup();
    static void Loop();
    static bool buttonA();
};