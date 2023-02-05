#pragma once
#ifdef SENSORS
#include <ArduinoJson.h>

namespace TVOC_SGP30 {
void ConnectToWifi();
void SerialReport();
bool SendDiscovery();
void Setup();
void Loop(uint32_t);
}  // namespace TVOC_SGP30

#endif