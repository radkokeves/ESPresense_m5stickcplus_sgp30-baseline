#include "BleFingerprint.h"
#include "util.h"

BleFingerprint::BleFingerprint(BLEAdvertisedDevice *advertisedDevice, float fcmin, float beta, float dcutoff) : oneEuro{one_euro_filter<double, unsigned long>(1, fcmin, beta, dcutoff)}
{
    if (advertisedDevice->getAddressType() == BLE_ADDR_PUBLIC)
        macPublic = true;

    firstSeenMicros = esp_timer_get_time();
    address = advertisedDevice->getAddress();
    newest = recent = oldest = rssi = advertisedDevice->getRSSI();

    calRssi = advertisedDevice->haveTXPower() ? (-advertisedDevice->getTXPower()) - 41 : 0;

    fingerprint(advertisedDevice);

    if (calRssi > 0) calRssi = defaultTxPower;
}

void BleFingerprint::fingerprint(BLEAdvertisedDevice *advertisedDevice)
{
    if (advertisedDevice->haveName())
        name = String(advertisedDevice->getName().c_str());

    if (advertisedDevice->haveServiceUUID())
    {
        if (advertisedDevice->isAdvertisingService(tileUUID))
        {
            pid = "tile:" + getMac();
        }
        else if (advertisedDevice->isAdvertisingService(exposureUUID))
        { // found covid exposure tracker
            std::string strServiceData = advertisedDevice->getServiceData(exposureUUID);
            pid = "exp:" + String(strServiceData.length());
        }
        else if (advertisedDevice->isAdvertisingService(sonosUUID))
        {
            pid = "sonos:" + getMac();
        }
        else if (advertisedDevice->isAdvertisingService(eddystoneUUID))
        { // found Eddystone UUID
            std::string strServiceData = advertisedDevice->getServiceData(eddystoneUUID);
            if (strServiceData[0] == EDDYSTONE_URL_FRAME_TYPE && strServiceData.length() <= 18)
            {
                BLEEddystoneURL oBeacon = BLEEddystoneURL();
                oBeacon.setData(strServiceData);
                url = String(oBeacon.getDecodedURL().c_str());
                calRssi = oBeacon.getPower();
            }
            else if (strServiceData[0] == EDDYSTONE_TLM_FRAME_TYPE)
            {
                BLEEddystoneTLM oBeacon = BLEEddystoneTLM();
                oBeacon.setData(strServiceData);
                temp = oBeacon.getTemp();
                volts = oBeacon.getVolt();
#ifdef VERBOSE
                Serial.println(oBeacon.toString().c_str());
#endif
            }
        }
        else
        {
            String fingerprint = "sid:";
            for (int i = 0; i < advertisedDevice->getServiceUUIDCount(); i++)
            {
                std::string sid = advertisedDevice->getServiceUUID(i).toString();
                fingerprint = fingerprint + String(sid.c_str());
            }
            if (advertisedDevice->haveTXPower())
                fingerprint = fingerprint + String(-advertisedDevice->getTXPower());
            sid = fingerprint;
        }
    }
    else if (advertisedDevice->haveManufacturerData())
    {
        std::string strManufacturerData = advertisedDevice->getManufacturerData();
        if (strManufacturerData.length() > 2)
        {
            String manuf = Sprintf("%02x%02x", strManufacturerData[1], strManufacturerData[0]);

            if (manuf == "004c") // Apple
            {
                if (strManufacturerData.length() == 25 && strManufacturerData[2] == 0x02 && strManufacturerData[3] == 0x15)
                {
                    BLEBeacon oBeacon = BLEBeacon();
                    oBeacon.setData(strManufacturerData);
                    pid = Sprintf("iBeacon:%s-%d-%d", std::string(oBeacon.getProximityUUID()).c_str(), ENDIAN_CHANGE_U16(oBeacon.getMajor()), ENDIAN_CHANGE_U16(oBeacon.getMinor()));
                    calRssi = oBeacon.getSignalPower();
                }
                else
                {
                    if (advertisedDevice->haveTXPower())
                        pid = Sprintf("apple:%02x%02x:%d%d", strManufacturerData[2], strManufacturerData[3], strManufacturerData.length(), -advertisedDevice->getTXPower());
                    else
                        pid = Sprintf("apple:%02x%02x:%d", strManufacturerData[2], strManufacturerData[3], strManufacturerData.length());
                }
            }
            else if (manuf == "05a7") //Sonos
            {
                pid = "sonos:" + getMac();
            }
            else if (manuf == "0157") //Mi-fit
            {
                pid = "mifit:" + getMac();
            }
            else if (manuf == "0006" && strManufacturerData.length() == 29) //microsoft
            {
                pid = Sprintf("microsoft:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                              strManufacturerData[6], strManufacturerData[7], strManufacturerData[8], strManufacturerData[9], strManufacturerData[10], strManufacturerData[11],
                              strManufacturerData[12], strManufacturerData[13], strManufacturerData[14], strManufacturerData[15], strManufacturerData[16], strManufacturerData[17],
                              strManufacturerData[18], strManufacturerData[19], strManufacturerData[20], strManufacturerData[21], strManufacturerData[22], strManufacturerData[23],
                              strManufacturerData[24], strManufacturerData[25], strManufacturerData[26], strManufacturerData[27], strManufacturerData[28]);
            }
            else if (manuf == "0075") //samsung
            {
                pid = "samsung:" + getMac();
            }
            else
            {
                String fingerprint = Sprintf("md:%s:%d", manuf.c_str(), strManufacturerData.length());
                if (advertisedDevice->haveTXPower())
                    fingerprint = fingerprint + String(-advertisedDevice->getTXPower());
                sid = fingerprint;
            }
        }
    }
}

bool BleFingerprint::filter()
{
    Reading<float> inter1, inter2;

    if (tsFilter.push(&raw, &inter1))
    {
        inter2.timestamp = inter1.timestamp;
        inter2.value = oneEuro(inter1.value);
        if (diffFilter.push(&inter2, &output))
            return true;
    }
    return false;
}

void BleFingerprint::seen(BLEAdvertisedDevice *advertisedDevice)
{
    lastSeenMicros = esp_timer_get_time();

    oldest = recent;
    recent = newest;
    newest = advertisedDevice->getRSSI();
    rssi = median_of_3(oldest, recent, newest);

    fingerprint(advertisedDevice);
    if (!calRssi) calRssi = defaultTxPower;

    float ratio = (calRssi - rssi) / 35.0f;
    raw = pow(10, ratio);

    if (filter())
    {
        hasValue = true;
        reported = false;
    }
}

void BleFingerprint::setInitial(int initalRssi, float initalDistance)
{
    newest = recent = oldest = rssi = initalRssi;
    raw = initalDistance;
    hasValue = filter() || filter();
}

bool BleFingerprint::report(JsonDocument *doc, int maxDistance)
{
    if (pid.isEmpty() && sid.isEmpty() && !macPublic)
        return false;

    if (!hasValue)
        return false;

    if (maxDistance > 0 && output.value.position > maxDistance)
        return false;

    if (reported)
        return false;

    auto now = esp_timer_get_time();

    if (abs(output.value.position - lastReported) < 0.1f && abs(now - lastReportedMicros) < 15000000)
        return false;

    lastReportedMicros = now;
    lastReported = output.value.position;
    reported = true;

    String mac = SMacf(address);
    if (output.value.position < 0.5)
    {
        if (!close)
        {
            //Display.close(mac.c_str(), id.c_str());
            close = true;
        }
    }
    else if (close && output.value.position > 1.5)
    {
        //Display.left(mac.c_str(), id.c_str());
        close = false;
    }

    (*doc)[F("id")] = getId();
    if (!name.isEmpty()) (*doc)[F("name")] = name;

    (*doc)[F("rssi@1m")] = calRssi;
    (*doc)[F("rssi")] = rssi;

    (*doc)[F("mac")] = mac;
    (*doc)[F("raw")] = round(raw * 100.0f) / 100.0f;
    (*doc)[F("distance")] = round(output.value.position * 100.0f) / 100.0f;
    (*doc)[F("speed")] = round(output.value.speed * 1e7f) / 10.0f;

    if (volts) (*doc)[F("volts")] = volts;
    if (temp) (*doc)[F("temp")] = temp;

    return true;
}
