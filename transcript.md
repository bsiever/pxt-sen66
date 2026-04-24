# Transcript of session below

User: Hi -- I'd like to update this for the Sensirion SEN66 from the 55.  The structure should be updated:
* Files renamed as appropriate
* Configurations and descriptions updated as appropriate. 
* Then we should review code and update as needed.

GitHub Copilot: Created 4 todos

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/pxt.json)

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen55.cpp#1-1), lines 1 to 100

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen55.cpp#100-100), lines 100 to 400

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen55.ts#1-1), lines 1 to 200

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen55.cpp#400-400), lines 400 to 479

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen55.ts#200-200), lines 200 to 354

Now let me check the README and look up SEN66 differences from SEN55:  



Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/README.md#1-1), lines 1 to 100

Fetched https://sensirion.com/resource/datasheet/sen66

Let me fetch the actual SEN66 datasheet and product page: 



Fetched 2 resources



Fetched 2 resources



Fetched https://raw.githubusercontent.com/Sensirion/arduino-i2c-sen66/main/src/SensirionI2cSen66.cpp



Fetched https://raw.githubusercontent.com/Sensirion/arduino-i2c-sen66/main/src/SensirionI2cSen66.h

Now I have everything I need. Let me check the remaining files:   

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/shims.d.ts)

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/enums.d.ts)

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/test.ts#1-1), lines 1 to 20

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/test.ts#20-20), lines 20 to 50

I now have a complete picture of all changes needed. Let me proceed. 

Starting: *Rename files from sen55 to sen66* (2/6)

Ran terminal command:  mv sen55.cpp sen66.cpp && mv sen55.ts sen66.ts

Starting: *Update pxt.json and enums/shims* (3/6)

Now update pxt.json, enums.d.ts, and shims.d.ts: 

Replacing 64 lines with 63 lines in [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/pxt.json), [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/enums.d.ts), [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/shims.d.ts)


```

```

```

```

```

```


Starting: *Rewrite sen66.cpp for SEN66* (4/6)

Now rewrite sen66.cpp with all SEN66-specific changes: 

Created [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen66.cpp)

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen66.cpp#1-1), lines 1 to 10

I'll rewrite the entire cpp file by replacing all content: 

Ran terminal command:  cat > /Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen66.cpp << 'ENDOFFILE'
/**
* Bill Siever
* 2024 Initial Version (adapted from SEN55 driver)
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
ENDOFFILE

Now rewrite sen66.ts: 

Ran terminal command:  cat > /Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/sen66.ts << 'ENDOFFILE'

enum Sen66ParticleMasses {
    //% block="1.0"
    PM10,
    //% block="2.5"
    PM25,
    //% block="4.0"
    PM40,
    //% block="10.0"
    PM100
}

enum Sen66ParticleCounts {
    //% block="0.5"
    PC05,
    //% block="1.0"
    PC10,
    //% block="2.5"
    PC25,
    //% block="4.0"
    PC40,
    //% block="10.0"
    PC100
}

/**
 * Support for the Sensirion SEN66 Environmental Sensor Node
 */
//% color=#398BB4
//% icon="\uf0c2"
//% block="SEN66"
namespace sen66 {

    export enum StatusMasks {
        //% block="fan speed warning"
        FanSpeedWarning = 1 << 21,
        //% block="PM sensor failure"
        PMFailure = 1 << 11,
        //% block="CO2 sensor failure"
        CO2Failure = 1 << 9,
        //% block="gas sensor failure"
        GasFailure = 1 << 7,
        //% block="temperature and humidity failure"
        RHTFailure = 1 << 6,
        //% block="fan failure"
        FanFailure = 1 << 4,
    }

    //% whenUsed
    let errorHandler: Action = null;
    //% whenUsed
    let _lastError: string = null;

    // ************** Private helpers **************

    //% shim=sen66::setErrorHandler
    function setErrorHandler(a: Action) {
        errorHandler = a; 
    }

    //% shim=sen66::startMeasurements
    function _startMeasurements() {
        0;
    }

    //% shim=sen66::lastError
    function lastError(): string {
        return _lastError;
    }

    // ************** Exposed primary blocks **************

    /**
      * Start Measurements.
      * Measurements must be started before data can be read. The sensor requires a warm-up period before measurements are accurate.
      * See the SEN66 datasheet for details: https://sensirion.com/products/catalog/SEN66/
      */
    //% block="start measurements"
    //% weight=1000
    export function startMeasurements() {
        _startMeasurements();
    }

    /**
     * Get the mass of particles that are up to the given size. Returns NaN on error. 
     * @param size the upper limit on particle size to include (defaults to 10µm)
     * Measurements must be started before use.
     */
    //% block="mass of particles from 0.3 to $size µm in µg/m³"
    //% shim=sen66::particleMass
    //% weight=700
    export function particleMass(size?: Sen66ParticleMasses) : number {
        // If undefined, default to largest (includes others)
        if(size == undefined) 
          size = Sen66ParticleMasses.PM100
        switch(size) {
            case Sen66ParticleMasses.PM10:
                return 1.0
            case Sen66ParticleMasses.PM25:
                return 2.5
            case Sen66ParticleMasses.PM40:
                return 4.0
            case Sen66ParticleMasses.PM100:
                return 10.0
        }
        return NaN;
    }


    /**
     * Get the count of particles that are up to the given size. Returns NaN on error.
     * @param size the upper limit on particle size to include (defaults to 10µm)
     * Measurements must be started before use.
     */
    //% block="count of particles from 0.3 to $size µm in #/cm³"
    //% shim=sen66::particleCount
    //% weight=650
    export function particleCount(size?: Sen66ParticleCounts) : number {
        if (size == undefined)
            size = Sen66ParticleCounts.PC100
        switch(size) {
            case Sen66ParticleCounts.PC05:
                return 0.5
            case Sen66ParticleCounts.PC10:
                return 1.0
            case Sen66ParticleCounts.PC25:
                return 2.5
            case Sen66ParticleCounts.PC40:
                return 4.0
            case Sen66ParticleCounts.PC100:
                return 10.0
        }
        return NaN;
    }

    /**
     * Get the CO2 concentration in ppm. Returns NaN on error.
     * Note: CO2 is not available for the first 5-6 seconds after power-on or reset.
     * Measurements must be started.
     */
    //% block="CO₂ concentration (ppm)"
    //% shim=sen66::CO2
    //% weight=620
    export function CO2(): number {
        return 400;
    }

    /**
     * Get the Sensirion VOC index. Returns NaN on error. See: https://sensirion.com/resource/application_note/voc_index
     * Value is from [1-500]
     * Measurements must be started.
     */
    //% block="VOC index"
    //% shim=sen66::VOC 
    //% weight=600
    export function VOCIndex(): number {
        return 1;
    }

    /**
     * Get the Sensirion NOx index. Returns NaN on error. See: https://sensirion.com/resource/application_note/nox_index
     * Value is from [1-500]
     * Measurements must be started.
     */
    //% block="NOx index"
    //% shim=sen66::NOx
    //% weight=500
    export function NOxIndex(): number {
        return 1;
    }

    /**
     * Get the temperature in °C, compensated based on Sensirion's STAR Engine. Returns NaN on error.
     * Measurements must be started.
     */
    //% block="temperature °C"
    //% shim=sen66::temperature
    //% weight=400
    export function temperature(): number {
        return 26.1;
    }

    /**
     * Get the relative humidity [0-100], compensated based on Sensirion's STAR Engine. Returns NaN on error.
     * Measurements must be started.
     */
    //% block="humidity (\\% relative)"
    //% shim=sen66::humidity
    //% weight=300
    export function humidity(): number {
        return 30.5;
    }

    /**
     * Stop all measurements. 
     */
    //% block="stop measurements"
    //% shim=sen66::stopMeasurements
    //% weight=200
    export function stopMeasurements() {
        0;
    }

    /**
     * Set a handler for errors 
     * @param errCallback The error handler 
     */
    //% blockId="error" block="SEN66 error"
    //% draggableParameters="reporter" weight=0
    //% weight=100
    export function onError(errCallback: (reason: string) => void) {
        if (errCallback) {
            errorHandler = () => {
                let le = lastError();
                errCallback(le);
            };
        } else {
            errorHandler = null;
        }
        setErrorHandler(errorHandler);
    }


    // ************** Exposed ADVANCED blocks **************

    /**
     * Get the device status. Returns a number with bit masks given in sen66.StatusMasks. Returns -1 on error. 
     */
    //% block="device status" advanced=true
    //% shim=sen66::deviceStatus
    //% weight=950
    export function deviceStatus(): number {
        return 0;
    }

    /**
     * Get the raw VOC value (not an index).  The raw value is proportional to the 
     * logarithm of the corresponding sensor resistance. Returns NaN on error.
     * Measurements must be started.
     */
    //% block="raw VOC ticks" advanced=true
    //% shim=sen66::rawVOC 
    //% weight=890
    export function rawVOC(): number {
        return 90;
    }


    /**
     * Get the raw NOx value (not an index).  The raw value is proportional to the 
     * logarithm of the corresponding sensor resistance. Returns NaN on error.
     * Measurements must be started.
     */
    //% block="raw NOx ticks" advanced=true
    //% shim=sen66::rawNOx
    //% weight=880
    export function rawNOx(): number {
        return 1;
    }

    /**
     * Get the raw (uninterpolated) CO2 concentration in ppm, updated every 5 seconds. Returns NaN on error.
     * Measurements must be started.
     */
    //% block="raw CO₂ (ppm)" advanced=true
    //% shim=sen66::rawCO2
    //% weight=875
    export function rawCO2(): number {
        return 400;
    }

    /**
     * Get the raw temperature value °C (not compensated). Returns NaN on error.
     * Measurements must be started.
     */
    //% block="raw temperature °C" advanced=true
    //% shim=sen66::rawTemperature
    //% weight=870
    export function rawTemperature(): number {
        return 26.1;
    }

    /**
     * Get the raw relative humidity value (not compensated). Returns NaN on error.
     * Measurements must be started.
     */
    //% block="raw humidity (\\% relative)" advanced=true
    //% shim=sen66::rawHumidity
    //% weight=860
    export function rawHumidity(): number {
        return 30.5;
    }

    /**
     * Get the product name. Returns an empty string on error.
     */
    //% block="product name" advanced=true
    //% shim=sen66::productName
    //% weight=830
    export function productName(): string {
        return "SEN66 (SIM)";
    }

    /**
     * Get the serial number.  Returns an empty string on error.
     */
    //% block="serial number" advanced=true
    //% shim=sen66::serialNumber
    //% weight=820
    export function serialNumber(): string {
        return "00";
    }

    /**
     * Get the firmware version.  Returns -1 on error.
     */
    //% block="firmware version" advanced=true
    //% shim=sen66::firmwareVersion
    //% weight=810
    export function firmwareVersion(): number {
        return 2;
    }

    /**
     * Reset the sensor (back to startup conditions; Not performing measurements)
     */
    //% block="reset" advanced=true
    //% shim=sen66::reset
    //% weight=600
    export function reset() {
        0;
    }

    /**
     * Clear the device status.
     */
    //% block="clear device status" advanced=true
    //% shim=sen66::clearDeviceStatus 
    //% weight=500
    export function clearDeviceStatus() {
        0;
    }

    /** 
     * Start cleaning the fan.
     * Takes ~10s and all values are invalid while cleaning. 
     * Fan will automatically be cleaned if the device is continuously running without reset/restart for 1 week (168 hours). 
     */
    //% block="start fan cleaning" advanced=true
    //% shim=sen66::startFanCleaning
    //% weight=400
    export function startFanCleaning() {
        0;
    }
}
ENDOFFILE

Starting: *Update test.ts and README.md* (6/6)

Now update test.ts and README.md:  

Ran terminal command:  cat > /Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/test.ts << 'ENDOFFILE'
sen66.onError(function (reason) {
    serial.writeLine(reason)
})

basic.showNumber(3)

input.onButtonPressed(Button.A, function () {
    serial.writeValue("pm10", sen66.particleMass(Sen66ParticleMasses.PM10))
    serial.writeValue("pm25", sen66.particleMass(Sen66ParticleMasses.PM25))
    serial.writeValue("pm40", sen66.particleMass(Sen66ParticleMasses.PM40))
    serial.writeValue("pm100", sen66.particleMass(Sen66ParticleMasses.PM100))
    pause(100)
    serial.writeValue("temp", sen66.temperature())
    serial.writeValue("humidity", sen66.humidity())
    serial.writeValue("VOC", sen66.VOCIndex())
    serial.writeValue("NOx", sen66.NOxIndex())
    serial.writeValue("CO2", sen66.CO2())
    pause(100)
    serial.writeValue("rawtemp", sen66.rawTemperature())
    serial.writeValue("rawhumidity", sen66.rawHumidity())
    serial.writeValue("rawNOx", sen66.rawNOx())
    serial.writeValue("rawVOC", sen66.rawVOC())
    serial.writeValue("rawCO2", sen66.rawCO2())
    pause(100)

    serial.writeValue("nc0p5", sen66.particleCount(Sen66ParticleCounts.PC05))
    serial.writeValue("nc1p0", sen66.particleCount(Sen66ParticleCounts.PC10))
    serial.writeValue("nc2p5", sen66.particleCount(Sen66ParticleCounts.PC25))
    pause(100)
    serial.writeValue("nc4p0", sen66.particleCount(Sen66ParticleCounts.PC40))
    serial.writeValue("nc10p0", sen66.particleCount(Sen66ParticleCounts.PC100))
    pause(100)
    serial.writeLine("-------------")

}) 
input.onButtonPressed(Button.B, function () {
    serial.writeLine(sen66.productName())
    serial.writeLine(sen66.serialNumber())
    serial.writeLine("" + (sen66.firmwareVersion()))
    let stat = sen66.deviceStatus()
    serial.writeLine("Status: " + stat)
    serial.writeLine("Fan status " + ((stat & sen66.StatusMasks.FanFailure) == 0) ? "off" : "on");
    sen66.startFanCleaning()
})
sen66.startMeasurements()
ENDOFFILE

Now update the README: 

Read [](file:///Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/README.md#100-100), lines 100 to 286

Ran terminal command:  cat > /Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/README.md << 'ENDOFFILE'

```package
sen66=github:bsiever/pxt-sen66
```

# SEN66 Air Quality Sensor

This extension supports the [Sensirion SEN66](https://sensirion.com/products/catalog/SEN66/) Environmental Sensor Node, which includes sensors for:

* Airborne Particulate Matter (Particle sizes of up to 1.0, 2.5, 4.0, and 10.0 µm)
* Volatile Organic Compounds (VOCs)
* Nitrogen Oxides (NOx)
* CO₂ (Carbon Dioxide)
* Relative Humidity
* Temperature

Sensor details and data sheets can be found at: [https://sensirion.com/products/catalog/SEN66/](https://sensirion.com/products/catalog/SEN66/)

# Parts & Wiring

## Parts 

Almost all the parts necessary for this project can be purchased from DigiKey:

1. Sensirion SEN66: https://www.digikey.com/en/products/detail/sensirion-ag/SEN66-SIN-T/22533887
2. A MicroBit Breakout Board: https://www.digikey.com/en/products/detail/sparkfun-electronics/BOB-16446/14557733

There are also a few parts that are available in a variety of kits and from other sources.  Below are variations that are available via Amazon:

1. A breadboard (only one needed)
2. Jumper wires
3. A 3.3V power supply (or use the 3.3V pin from the micro:bit breakout).

## Wiring

The SEN66 uses a **3.3V** supply (unlike the SEN55 which required 5V).

The SEN66 pin layout (looking at the connector):

| Pin | Color (typical) | Signal | Notes |
|-----|-----------------|--------|-------|
| 1 | Red   | VDD | 3.3V supply |
| 2 | Black | GND | Ground |
| 3 | Green | SDA | I2C data (TTL 5V compatible) |
| 4 | Yellow | SCL | I2C clock (TTL 5V compatible) |
| 5 | — | NC | Do not connect (internally tied to GND) |
| 6 | — | NC | Do not connect (internally tied to VDD) |

The I2C address of the SEN66 is **0x6B**.

Connect:
1. SEN66 Pin 1 (VDD) to the 3.3V rail.
2. SEN66 Pin 2 (GND) to ground.
3. SEN66 Pin 3 (SDA) to the micro:bit SDA (Pin 20).
4. SEN66 Pin 4 (SCL) to the micro:bit SCL (Pin 19).
5. Leave Pins 5 and 6 unconnected.

### ~alert

# Errors on Numeric Values

Errors when reading numeric values are reported as `NaN` ("Not a Number").  `Number.isNaN()` can be used to determine if the returned value is not valid (is `NaN`).
### ~


# Start Measurements

```sig
sen66.startMeasurements() : void
```

Start making measurements. Measurements must be started before data can be read. The sensor requires a warm-up period before measurements are accurate.

# Mass of particles by particle size

```sig
sen66.particleMass(size?: Sen66ParticleMasses) : number
```

Get the mass of particles of size 0.3µm up to the given size per volume (µg/m³). Returns `NaN` on error.

Measurements must be started via `sen66.startMeasurements()` prior to use.  

# Count of particles by particle size

```sig
sen66.particleCount(size?: Sen66ParticleCounts) : number
```

Get the count of particles of size 0.3µm up to the given size per volume (#/cm³).

Measurements must be started via `sen66.startMeasurements()` prior to use.  Returns `NaN` on error.

# CO2 Concentration

```sig
sen66.CO2(): number
```

Get the CO₂ concentration in ppm [0-40000]. Returns `NaN` on error.
Note: CO₂ is not available for the first 5-6 seconds after power-on or reset.

# VOC Index 

```sig
sen66.VOCIndex(): number
```

Get the VOC index [1-500]. See https://sensirion.com/media/documents/02232963/6294E043/Info_Note_VOC_Index.pdf .  Returns `NaN` on error.

# NOx Index 

```sig
sen66.NOxIndex(): number
```

Get the NOx index [1-500]. See https://sensirion.com/media/documents/9F289B95/6294DFFC/Info_Note_NOx_Index.pdf .  Returns `NaN` on error.

# Temperature 

```sig
sen66.temperature(): number
```

Get the temperature in Celsius.  The temperature value will be compensated based on Sensirion's STAR algorithm.

# Humidity

```sig
sen66.humidity(): number
```

Get the relative humidity (0-100%).  The humidity value will be compensated based on Sensirion's STAR algorithm.  Returns `NaN` on error.

# Stop Measurements

```sig
sen66.stopMeasurements() : void
```

Stop making measurements and return to low-power idle mode.

# On Error 

```sig
sen66.onError(errCallback: (reason: string) => void) : void
```

Respond to any errors.  `reason` will be a description of the error. 

# Device Status

```sig
sen66.deviceStatus(): number
```

Get the device status. Returns a number with bit masks given in `sen66.StatusMasks`. Returns -1 on error.

# Raw VOC

```sig
sen66.rawVOC(): number
```

Get the raw VOC ticks (not an index).  The raw value is proportional to the logarithm of the corresponding sensor resistance. Returns `NaN` on error.

# Raw NOx

```sig
sen66.rawNOx(): number
```

Get the raw NOx ticks (not an index).  The raw value is proportional to the logarithm of the corresponding sensor resistance. Returns `NaN` on error.

# Raw CO2

```sig
sen66.rawCO2(): number
```

Get the raw (uninterpolated) CO₂ concentration in ppm, updated every 5 seconds. Returns `NaN` on error.

# Raw Temperature

```sig
sen66.rawTemperature(): number
```

Get the raw temperature value °C (not compensated). Returns `NaN` on error.

# Raw Humidity

```sig
sen66.rawHumidity(): number
```

Get the raw relative humidity (not compensated). Returns `NaN` on error.

# Product Name

```sig
sen66.productName(): string
```

Get the product name. Returns an empty string on error.

# Serial Number

```sig
sen66.serialNumber(): string
```

Get the Serial number. Returns an empty string on error.

# Firmware Version

```sig
sen66.firmwareVersion(): number
```

Get the firmware version.  Returns -1 on error.

# Reset

```sig
sen66.reset(): void
```

Reset the sensor (back to startup conditions; Not performing measurements).

# Clear Device Status

```sig
sen66.clearDeviceStatus(): void
```

Clear the device status.

# Start Fan Cleaning

```sig
sen66.startFanCleaning(): void
```

Start cleaning the fan. Takes ~10s and all values are invalid while cleaning. 

Fan will automatically be cleaned if the device is continuously running without reset/restart for 1 week (168 hours).

# Example

The following program will get air quality measures every second and relay them to the serial console / logger.
```block

basic.showIcon(IconNames.Heart)
sen66.startMeasurements()

sen66.onError(function (reason) {
    serial.writeLine(reason)
})

loops.everyInterval(1000, function () {
    serial.writeValue("pm10", sen66.particleMass(Sen66ParticleMasses.PM100))
    serial.writeValue("co2", sen66.CO2())
    serial.writeValue("voc", sen66.VOCIndex())
    serial.writeValue("NOx", sen66.NOxIndex())
    serial.writeValue("temp", sen66.temperature())
    serial.writeValue("rh", sen66.humidity())
})


``` 


# Acknowledgements 

Icon based on [Font Awesome icon 0xf0c2](https://www.iconfinder.com/search?q=f0c2) SVG.

# Misc. 

I develop micro:bit extensions in my spare time to support activities I'm enthusiastic about, like summer camps and science curricula.  You are welcome to become a sponsor of my micro:bit work (one time or recurring payments), which helps offset equipment costs: [here](https://github.com/sponsors/bsiever). Any support at all is greatly appreciated!

## Supported targets

for PXT/microbit

<script src="https://makecode.com/gh-pages-embed.js"></script>
<script>makeCodeRender("{{ site.makecode.home_url }}", "{{ site.github.owner_name }}/{{ site.github.repository_name }}");</script>
ENDOFFILE

Completed: *Update test.ts and README.md* (6/6)

Let me verify the final file structure looks correct: 

Ran terminal command:  ls /Users/bsiever/Desktop/Projects/microbit/extensions/sen66/pxt-sen66/

Made changes.