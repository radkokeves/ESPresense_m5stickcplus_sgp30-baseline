#ifdef SENSORS
#include "TVOC_SGP30.h"

#include <AsyncWiFiSettings.h>

#include <M5StickCPlus.h>
#include <Adafruit_SGP30.h>

#define ESPFS SPIFFS

#include <SPIFFS.h>

#include <Wire.h>

#include "defaults.h"
#include "globals.h"
#include "mqtt.h"
#include "string_utils.h"

//#include "I2C.h"

#include "defaults.h"
#include "globals.h"
#include "mqtt.h"
#include "string_utils.h"
#include "Display.h"

namespace TVOC_SGP30 {

String Adafruit_SGP30_I2c;
int Adafruit_SGP30_I2c_Bus;

bool initialized = false;
unsigned long SGP30PreviousMillis = 0;
unsigned long lastBaselineMillis=0;
int sensorInterval = 1000;
long SGP30_status;
String baselineFn = "/SGP30bl";

Adafruit_SGP30 sgp;

int i            = 15;
long last_millis = 0;

String readBaseline(const String &fn) {
    //Serial.println("SGP30: baseline file: "+fn+" "+ESPFS.exists(fn));
    if (File f = ESPFS.open(fn, "r")) {
        String r = f.readString();
        f.close();
        Serial.println("SGP30: reading baseline "+fn+": "+r);
        return r;
    } else {
        Serial.println("SGP30: baseline file "+fn+": not found");
    }
    return "";
}

bool saveBaseline(const String &fn, uint16_t base) {
    //Serial.println("SGP30: saveBaseline()");
    char buffer[10];
    sprintf(buffer, "%u\n", (unsigned int)base);
    bool res = spurt(fn, buffer);
    Serial.print("SGP30: saving baseline "+fn+":"+res+":");
    Serial.printf("%u\n", (unsigned int)base);
    return res;
}

/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

void Setup() {

    if (Adafruit_SGP30_I2c.isEmpty()) {
        Serial.println("SGP30: Setup() Adafruit_SGP30_I2c.isEmpty()");
        return;
    }
    if (!I2C_Bus_1_Started && !I2C_Bus_2_Started) return;

    Serial.begin(115200);
    while (!Serial) { delay(10); } // Wait for serial console to open!
    Wire.begin(32, 33);
    Serial.println("SGP30: ... waiting 15 seconds for initialization");

    delay(15000);

    if (Adafruit_SGP30_I2c == "0x58") {
        //SGP30_status = sgp.begin(Adafruit_SGP30_I2c_Bus == 1 ? Wire : Wire1, (bool)true);
        SGP30_status = sgp.begin();
        if (! SGP30_status){
            Serial.println("SGP30: Sensor not found :(");
            return;            
        }
    } else {
        Serial.println("SGP30: error: Adafruit_SGP30_I2c != 0x58");
        return;
    }

    Serial.print("SGP30: Found SGP30 serial #");
    Serial.print(sgp.serialnumber[0], HEX);
    Serial.print(sgp.serialnumber[1], HEX);
    Serial.println(sgp.serialnumber[2], HEX);

    initialized = true;

    String eco2_base_string = readBaseline(baselineFn+"eco2_base");
    String tvoc_base_string = readBaseline(baselineFn+"tvoc_base");
    //Serial.println(baselineFn+"eco2_base.out"+":"+eco2_base_string);
    //Serial.println(baselineFn+"tvoc_base.out"+":"+tvoc_base_string);
    uint16_t TVOC_base, eCO2_base;
    if (!eco2_base_string.length() == 0 && !tvoc_base_string.length() == 0 ) {
        uint16_t eCO2_base = strtol(eco2_base_string.c_str(), NULL, 0);
        uint16_t TVOC_base = strtol(tvoc_base_string.c_str(), NULL, 0);
        sgp.setIAQBaseline(eCO2_base, TVOC_base);
        Serial.print("SGP30: reading baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
        Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
    } else {
        Serial.println("SGP30: baseline not found");    
    }
}

void ConnectToWifi() {
    //XXX:TODO currently ignoring actual values using it only as placeholder
    AsyncWiFiSettings.html("h4", "Adafruit SGP30 - Air Quality Sensor:");
    Adafruit_SGP30_I2c_Bus = AsyncWiFiSettings.integer("Adafruit_SGP30_I2c_Bus", 1, 2, DEFAULT_I2C_BUS, "I2C Bus");
    Adafruit_SGP30_I2c = AsyncWiFiSettings.string("Adafruit_SGP30_I2c", "", "I2C address (0x58)");
}

void SerialReport() {
    if (!I2C_Bus_1_Started && !I2C_Bus_2_Started) return;
    if (Adafruit_SGP30_I2c.isEmpty()) {
        Serial.print("SGP30: SerialReport() Adafruit_SGP30_I2c.isEmpty()");
        return;
    }
    Serial.print("SGP30:        ");
    Serial.println("Adafruit_SGP30_I2c:"+Adafruit_SGP30_I2c + " on bus " + Adafruit_SGP30_I2c_Bus);
}

int counter = 0;
void Loop() {

    if (!initialized) return;

    if (SGP30PreviousMillis == 0 || millis() - SGP30PreviousMillis >= sensorInterval) {
        SGP30PreviousMillis = millis();

        if (! sgp.IAQmeasure()) {
            Serial.println("SGP30: IAQmeasurement failed");
            return;
        }
        /*
        Serial.print("TVOC "); Serial.print(sgp.TVOC); Serial.print(" ppb\t");
        Serial.print("eCO2 "); Serial.print(sgp.eCO2); Serial.println(" ppm");
        */
       Display::Status("TVOC %i ppb\n",sgp.TVOC);
       Display::Status("eCO2 %i ppm\n",sgp.eCO2);

        if (! sgp.IAQmeasureRaw()) {
            Serial.println("SGP30: Raw Measurement failed");
            return;
        }
        /*
        Serial.print("Raw H2 "); Serial.print(sgp.rawH2); Serial.print(" \t");
        Serial.print("Raw Ethanol "); Serial.print(sgp.rawEthanol); Serial.println("");
        */
        //delay(1000);
        if (SGP30PreviousMillis > 30000) {  // First 30 seconds after boot, don't report
            pub((roomsTopic + "/eco2").c_str(), 0, 1, String(sgp.eCO2).c_str());
            pub((roomsTopic + "/tvoc").c_str(), 0, 1, String(sgp.TVOC).c_str());
            pub((roomsTopic + "/rawh2").c_str(), 0, 1, String(sgp.rawH2).c_str());
            pub((roomsTopic + "/rawethanol").c_str(), 0, 1, String(sgp.rawEthanol).c_str());            
        }
        if (millis() - lastBaselineMillis >= 12*60*60*1000UL) { //12 hours to establish baseline  
            lastBaselineMillis = millis();
            uint16_t TVOC_base, eCO2_base;
            if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
                Serial.println("SGP30: Failed to get baseline readings");
                return;
            }

            saveBaseline(baselineFn+"eco2_base",eCO2_base);
            saveBaseline(baselineFn+"tvoc_base",TVOC_base);

            Serial.print("SGP30: saving baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
            Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);

            pub((roomsTopic + "/eco2_base").c_str(), 0, 1, String(eCO2_base).c_str());
            pub((roomsTopic + "/tvoc_base").c_str(), 0, 1, String(TVOC_base).c_str());                     
        }
    }
}

bool SendDiscovery() {
    if (Adafruit_SGP30_I2c.isEmpty()) {
        Serial.println("SGP30: SendDiscovery() Adafruit_SGP30_I2c.isEmpty()");
        return true;
    }

    return sendSensorDiscovery("eCo2", EC_NONE, "carbon_dioxide", "ppm") && sendSensorDiscovery("TVOC", EC_NONE, "volatile_organic_compounds", "ppb")
        && sendSensorDiscovery("eco2_base", EC_NONE, "eco2_base", "ppb") && sendSensorDiscovery("tvoc_base", EC_NONE, "tvoc_base", "ppb")
        && sendSensorDiscovery("rawH2", EC_NONE, "rawH2", "ppb") && sendSensorDiscovery("rawEthanol", EC_NONE, "rawEthanol", "ppb");
}
}  // namespace TVOC_SGP30

#endif