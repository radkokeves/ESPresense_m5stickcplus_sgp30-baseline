#pragma once
#ifdef SENSORS
#include <ArduinoJson.h>

namespace m5envIII {
void ConnectToWifi();
void SerialReport();
bool SendDiscovery();
void Setup();
void Loop();
}  // namespace m5envIII

#endif