#pragma once

#include <Arduino.h>
#include "BleFingerprint.h"

class BleFingerprint;

class Display {
   public:
    static void Setup();
    static void ConnectToWifi();
    static void Loop();
    static void Status(const char *message, ...);
    static void Button();
};
