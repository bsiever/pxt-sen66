/**
* Bill Siever
* 2026 Initial Version (adapted from SEN55 driver - adapted by Co-Pilot)
*
* This code is released under the [MIT License](http://opensource.org/licenses/MIT).
* Please review the LICENSE.md file included with this example. If you have any questions 
* or concerns with licensing, please contact the author.
* Distributed as-is; no warranty is given.
*/
 
#include "pxt.h"
#include "MicroBitI2C.h"
#include "MicroBit.h"

using namespace pxt;


enum SEN66RunMode
{
    Idle,
    Measurement,
};

enum SEN66ParticleMasses
{
    PM10,
    PM25,
    PM40,
    PM100, 
    PMCount
};

enum SEN66ParticleCounts
{
    NC05,
    NC10,
    NC25,
    NC40,
    NC100,
    NCCount
};


namespace sen66 {

    // Sensirion SEN66 I2C address is 0x6B:
    const int address = 0x6B << 1;
    const int staleDataTime = 1050;   // ms until data is considered stale
    const uint16_t UINT_INVALID = (uint16_t)0xFFFF;
    const int16_t  INT_INVALID = (int16_t)0x7FFF;

    // State variables
    static Action errorHandler = NULL;
    static String _lastError = PSTR("");

    static SEN66RunMode mode = Idle; 
    static unsigned long _lastReadTime;
    static uint16_t _pm[PMCount];
    static int16_t  _rh;
    static int16_t  _temp;
    static int16_t  _voci;
    static int16_t  _noxi;
    static uint16_t _co2;
    static int16_t  _rawRh;
    static int16_t  _rawTemp; 
    static uint16_t _rawVoc;
    static uint16_t _rawNox;
    static uint16_t _rawCo2;

    static uint16_t _nc[NCCount];

    static void invalidateValues() {
        _lastReadTime = 0;  // No valid read has occurred
        for (int i = 0;i<PMCount;i++) {
            _pm[i] = UINT_INVALID;
        }
        _rh   = INT_INVALID;
        _temp = INT_INVALID;
        _voci = INT_INVALID;
        _noxi = INT_INVALID;
        _co2  = UINT_INVALID;
        _rawRh   = INT_INVALID;
        _rawTemp = INT_INVALID;
        _rawVoc  = UINT_INVALID;
        _rawNox  = UINT_INVALID;
        _rawCo2  = UINT_INVALID;

        for (int i = 0;i<NCCount;i++) {
            _nc[i] = UINT_INVALID;
        }
    }

    // Forward Decls
    int deviceStatus();

    /*
     * Helper method to send an actual error code to the registered handler.
     * It will call the handler
     */
    static void sen66_error(const char *error="") {
        _lastError = PSTR(error);
        if(errorHandler) {
            pxt::runAction0(errorHandler);            
        }
    }

    // CRC calculation  
    // Sensirion SEN66 Data Sheet
    // https://sensirion.com/resource/datasheet/sen6x
    static uint8_t CalcCrc(uint8_t data[2]) {
        uint8_t crc = 0xFF;
        for(int i = 0; i < 2; i++) {
            crc ^= data[i];
            for(uint8_t bit = 8; bit > 0; --bit) {
                if(crc & 0x80) {
                    crc = (crc << 1) ^ 0x31u;
                } else {
                    crc = (crc << 1);
                }
            }   
        }
        return crc; 
    }

    static inline float roundP1(float value) {
        // Round to nearest sixteenth  (0.0625)
        return round(value*16)/16.0f;
    }
    static inline float roundP01(float value) {
        // Round to nearest 1/128th  (0.0078125)
        return round(value*128)/128.0f;
    }

    /*
    * Check all the CRCs of a buffer (buffer length must be multiple of 3)
    * returns true if all CRCs are correct, false otherwise.
    */
    static bool checkBuffer(uint8_t *buffer, uint8_t length) {
        for (uint8_t i = 0; i < length; i += 3)
        {
            uint8_t crc = CalcCrc(buffer+i);
            if(crc != buffer[i+2]) {
                return false;
            }
        }
        return true;
    }

    // Send a command to the SEN66 and wait for the specified delay (in ms)
    static bool sendCommand(uint16_t command, uint16_t delay) {
        command = ((command & 0xFF00) >> 8) | ((command & 0x00FF) << 8);
        uint8_t status; 
#if MICROBIT_CODAL
        status = uBit.i2c.write(address, (uint8_t *)&command, 2);
#else
        status = uBit.i2c.write(address, (char *)&command, 2);
#endif
        uBit.sleep(delay);
        return status == MICROBIT_OK;
    }

    // Read data from the SEN66
    // Returns true if successful, false otherwise
    static bool readData(uint8_t *data, uint16_t length) {
        uint8_t status = 0;
#if MICROBIT_CODAL
        status = uBit.i2c.read(address, (uint8_t *)data, length);
#else
        status = uBit.i2c.read(address, (char *)data, length);
#endif
        return status == MICROBIT_OK;
    }

    // Both name and serial number use the same format
    static String read48ByteString(uint16_t command) {
        bool commandStatus = sendCommand(command, 20);
        uint8_t data[48];
        bool readStatus = readData(data, sizeof(data));
        if(commandStatus==false || readStatus==false || checkBuffer(data, sizeof(data))==false) {
            sen66_error("Read String Error");
            return PSTR("");
        }
        uint16_t loc = 0;
        // Squeeze all bytes in without CRCs
        for (uint16_t i = 0; i < sizeof(data)-2; i += 3) {
            data[loc++] = data[i];
            data[loc++] = data[i+1];
        }
        data[loc] = 0; // Ensure null termination

        return PSTR((char*)data);
    }

    /* 
     * Read the sensor values and update state variables
    */
    static void readValues() {
        if (deviceStatus() != 0)
        {
            invalidateValues();
            sen66_error("Read Sensor: Device Status Error");
            return;
        }

        // Read main measurement values (PM1, PM2.5, PM4, PM10, RH, Temp, VOC, NOx, CO2)
        // Command 0x0300, returns 27 bytes (9 values x 3 bytes each with CRC)
        uint8_t data[27];
        bool commandStatus = sendCommand(0x0300, 20);
        bool readStatus = readData(data, sizeof(data));
        if(mode==Idle || commandStatus==false || readStatus==false || checkBuffer(data, sizeof(data))==false) {
            invalidateValues();
            sen66_error("Read Sensor Data Error");
            return;
        }

        // Read number concentration values (NC0.5, NC1, NC2.5, NC4, NC10)
        // Command 0x0316, returns 15 bytes (5 values x 3 bytes each with CRC)
        uint8_t ncData[15];
        commandStatus = sendCommand(0x0316, 20);
        readStatus = readData(ncData, sizeof(ncData));
        if(commandStatus==false || readStatus==false || checkBuffer(ncData, sizeof(ncData))==false) {
            invalidateValues();
            sen66_error("Read Sensor NC Data Error");
            return;
        }

        // Read raw values (rawRH, rawTemp, rawVOC, rawNOx, rawCO2)
        // Command 0x0405, returns 15 bytes (5 values x 3 bytes each with CRC)
        uint8_t rawData[15];
        commandStatus = sendCommand(0x0405, 20);
        readStatus = readData(rawData, sizeof(rawData));
        if(commandStatus==false || readStatus==false || checkBuffer(rawData, sizeof(rawData))==false) {
            invalidateValues();
            sen66_error("Read Sensor Raw Data Error");
            return;
        }

        // Valid read: Update state
        _lastReadTime = uBit.systemTime();

        // Main values (each value is 2 bytes + 1 CRC byte)
        _pm[PM10]  = (uint16_t)(data[0]<<8  | data[1]);
        _pm[PM25]  = (uint16_t)(data[3]<<8  | data[4]);
        _pm[PM40]  = (uint16_t)(data[6]<<8  | data[7]);
        _pm[PM100] = (uint16_t)(data[9]<<8  | data[10]);
        _rh        = (int16_t) (data[12]<<8 | data[13]);
        _temp      = (int16_t) (data[15]<<8 | data[16]);
        _voci      = (int16_t) (data[18]<<8 | data[19]);
        _noxi      = (int16_t) (data[21]<<8 | data[22]);
        _co2       = (uint16_t)(data[24]<<8 | data[25]);

        // Number concentration values
        _nc[NC05]  = (uint16_t)(ncData[0]<<8  | ncData[1]);
        _nc[NC10]  = (uint16_t)(ncData[3]<<8  | ncData[4]);
        _nc[NC25]  = (uint16_t)(ncData[6]<<8  | ncData[7]);
        _nc[NC40]  = (uint16_t)(ncData[9]<<8  | ncData[10]);
        _nc[NC100] = (uint16_t)(ncData[12]<<8 | ncData[13]);

        // Raw values
        _rawRh   = (int16_t) (rawData[0]<<8  | rawData[1]);
        _rawTemp = (int16_t) (rawData[3]<<8  | rawData[4]);
        _rawVoc  = (uint16_t)(rawData[6]<<8  | rawData[7]);
        _rawNox  = (uint16_t)(rawData[9]<<8  | rawData[10]);
        _rawCo2  = (uint16_t)(rawData[12]<<8 | rawData[13]);
    }

    /* 
    * Read state if stored values are stale (more than a second old)
    */
    static void readIfStale() {
        if( (_lastReadTime==0) || ((uBit.systemTime() - _lastReadTime) > staleDataTime) ) {
            readValues();
        }
    }

    //************************ MakeCode Blocks ************************

    //% 
    void setErrorHandler(Action a) {
       // Release any prior error handler
       if(errorHandler)
         pxt::decr(errorHandler);
        errorHandler = a; 
        if(errorHandler)    
            pxt::incr(errorHandler);
    }

    //% 
    float temperature() {
        readIfStale();
        return _temp == INT_INVALID ? NAN : roundP01(_temp/200.0f);
    }

    //% 
    float humidity() {
        readIfStale();
        return _rh == INT_INVALID ? NAN : roundP01(_rh/100.0f);
    }

    //%
    float VOC() {
        readIfStale();
        return _voci == INT_INVALID ? NAN : roundP1(_voci/10.0f);
    }

    //%
    float NOx() {
        readIfStale();
        return _noxi == INT_INVALID ? NAN : roundP1(_noxi/10.0f);
    }

    //%
    float CO2() {
        readIfStale();
        return _co2 == UINT_INVALID ? NAN : (float)_co2;
    }

    //%
    float rawHumidity() {
        readIfStale();
        return _rawRh == INT_INVALID ? NAN : roundP01(_rawRh/100.0f);
    }

    //% 
    float rawTemperature() {
        readIfStale();
        return _rawTemp == INT_INVALID ? NAN : roundP01(_rawTemp/200.0f);
    }

    //%
    float rawVOC() {
        readIfStale();
        return _rawVoc == UINT_INVALID ? NAN : (_rawVoc/1.0f);
    }

    //% 
    float rawNOx() {
        readIfStale();
        return _rawNox == UINT_INVALID ? NAN : (_rawNox/1.0f);
    }

    //%
    float rawCO2() {
        readIfStale();
        return _rawCo2 == UINT_INVALID ? NAN : (float)_rawCo2;
    }

    // Get product name:  Returns null on communication failure. 
    //%
    String productName() {
        return read48ByteString(0xD014);
    }

    // Get serial number:  Returns null on communication failure.
    //%
    String serialNumber() {
        return read48ByteString(0xD033);
    }

    // Get firmware:  Returns firmware major version number.
    //%
    int firmwareVersion() {
        bool commandStatus =  sendCommand(0xD100, 20);
        uint8_t data[3];
        bool readStatus = readData(data, sizeof(data));
        if(commandStatus==false || readStatus==false || checkBuffer(data, sizeof(data))==false) {
            sen66_error("Read Firmware Error");
            return -1;
        }
        return data[0];
    }

    //%
    void startMeasurements() {
        bool commandStatus = sendCommand(0x0021, 50);
        if(commandStatus == false) {
            sen66_error("Start Measurements Error");
        } else {
            mode = Measurement;
        }
    }
   
    //%
    void stopMeasurements() {
        bool commandStatus = sendCommand(0x0104, 200);
        if(commandStatus == false) {
            sen66_error("Stop Measurements Error");
        } else {
            mode = Idle;
        }
    }

    //% 
    void reset() {
        bool commandStatus = sendCommand(0xD304, 100);
        if(commandStatus == false) {
            sen66_error("Reset Error");
        } else {
            mode = Idle;
            invalidateValues();
            _lastError = PSTR("");
        }
    }

    //%
    float particleMass(SEN66ParticleMasses mass) {
        if(mass < PM10 || mass > PM100) {
            return NAN;
        }
        readIfStale();
        return _pm[mass] == UINT_INVALID ? NAN : roundP1(_pm[mass]/10.0);
    }

    //% 
    float particleCount(SEN66ParticleCounts count) {
        if(count < NC05 || count > NC100) {
            return NAN;
        }
        readIfStale();
        return _nc[count] == UINT_INVALID ? NAN : roundP1(_nc[count]/10.0);
    }

    //% 
    void clearDeviceStatus() {
        // readAndClearDeviceStatus: command 0xD210 returns status then clears flags
        bool commandStatus = sendCommand(0xD210, 20);
        if(commandStatus == false) {
            sen66_error("Device Status Clear Error");
            return;
        }
        // Read and discard the returned status bytes
        uint8_t data[6];
        readData(data, sizeof(data));
    }

    //% 
    String lastError() {
        return _lastError;
    }

    //%
    int deviceStatus() {
        bool commandStatus = sendCommand(0xD206, 20);
        uint8_t data[6];
        bool readStatus = readData(data, sizeof(data));
        if(commandStatus==false || readStatus==false || checkBuffer(data, sizeof(data))==false) {
            sen66_error("Read Device Status Error");
            return -1;
        }
        uint32_t status = data[0] << 24 | data[1] << 16 | data[3] << 8 | data[4];
        if(status!=0) {
            if(status & 1<<21) {
                sen66_error("Device Status: Fan Speed Warning");
            }
            if(status & 1<<11) {
                sen66_error("Device Status: PM Sensor Error");
            }
            if(status & 1<<9) {
                sen66_error("Device Status: CO2 Sensor Error");
            }
            if(status & 1<<7) {
                sen66_error("Device Status: Gas Sensor Error");
            }
            if(status & 1<<6) {
                sen66_error("Device Status: Humidity/Temp Sensor Error");
            }
            if(status & 1<<4) {
                sen66_error("Device Status: Fan Error");
            }
        }
        return status;
    }

    //%
    void startFanCleaning() {
        bool commandStatus = sendCommand(0x5607, 20);
        if(commandStatus == false) {
            sen66_error("Start Fan Cleaning Error");
        } 
    }
}
