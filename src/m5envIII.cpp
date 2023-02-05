#ifdef SENSORS
#include "m5envIII.h"

#include "M5_ENV.h"
#include <AsyncWiFiSettings.h>

#include <M5StickCPlus.h>


#include <Wire.h>

#include "defaults.h"
#include "globals.h"
#include "mqtt.h"
#include "string_utils.h"


#include "defaults.h"
#include "globals.h"
#include "mqtt.h"
#include "string_utils.h"
#include "Display.h"

namespace m5envIII {

bool m5envIIIenabled;

bool initialized = false;
unsigned long previousMillis = 0;
unsigned long lastBaselineMillis=0;
int sensorInterval = 10000;

SHT3X sht30;
QMP6988 qmp6988;

float tmp      = 0.0;
float hum      = 0.0;
float pressure = 0.0;

long last_millis = 0;

void Setup() {
    if (m5envIIIenabled) {
        Serial.println("qmp6988 Setup() m5envIIIenabled");
        Wire.begin(32, 33);  // Wire init, adding the I2C bus. 
        uint8_t r = qmp6988.init();
        if (r < 1) {
            Serial.println("qmp6988 initialization failed");
            return;
        } else {
            //XXX:TODO
            initialized = true;
            Serial.println("qmp6988 initialized");
            //M5.lcd.println(F("ENVIII Unit(SHT30 and QMP6988) test"));
        }
    } else {
        Serial.println("qmp6988 Setup() NOT m5envIIIenabled");
    }
}

void ConnectToWifi() {
    //XXX:TODO currently ignoring actual values using it only as placeholder
    AsyncWiFiSettings.html("h4", "M5 ENVIII_SHT30_QMP6988:");
    m5envIIIenabled = AsyncWiFiSettings.checkbox("m5envIIIenabled", false, "enable M5 ENVIII_SHT30_QMP6988");
}

void SerialReport() {
/*
    if (!initialized) {
        Serial.println("qmp6988: SerialReport() not initialized");
        return;
    }
*/
}

uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

int counter = 0;
uint32_t Loop() {

    if (!initialized) return 0;

    if (previousMillis == 0 || millis() - previousMillis >= sensorInterval) {
        previousMillis = millis();

        pressure = qmp6988.calcPressure() / 100;
        if (sht30.get() == 0) {  // Obtain the data of shT30
            tmp = sht30.cTemp;   // Store the temperature obtained from shT30.
            hum = sht30.humidity;  // Store the humidity obtained from the SHT30.
        } else {
            tmp = 0, hum = 0;
        }
        
        Serial.printf("m5envIII Temp: %2.1f  \tHumi: %2.0f%%  \tPressure: %2.0fhPa\r\n", tmp, hum, pressure);
        Display::Status("Temp: %2.1f  \tHumi: %2.0f%%  \tPress: %2.0fhPa\r\n", tmp, hum, pressure);

        if (previousMillis > 30000) {  // First 30 seconds after boot, don't report
            pub((roomsTopic + "/temperature").c_str(), 0, 1, String(tmp).c_str());
            pub((roomsTopic + "/humidity").c_str(), 0, 1, String(hum).c_str());
            pub((roomsTopic + "/pressure").c_str(), 0, 1, String(pressure).c_str());
            return getAbsoluteHumidity(tmp, hum);
        }
    }
    return 0;
}

bool SendDiscovery() {
    if (!initialized) {
        Serial.println("qmp6988: SendDiscovery() not initialized");
        return true;
    }

    return sendSensorDiscovery("Temperature", EC_NONE, "temperature", "°C") && sendSensorDiscovery("Humidity", EC_NONE, "humidity", "%") && sendSensorDiscovery("BME280 Pressure", EC_NONE, "pressure", "hPa");
}

} //m5envIII

#endif