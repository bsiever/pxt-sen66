
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
