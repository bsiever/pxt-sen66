
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

1. Sensirion SEN66: https://www.digikey.com/en/products/detail/sensirion-ag/SEN66-SIN-T/25700945
2. A MicroBit Breakout Board: https://www.digikey.com/en/products/detail/sparkfun-electronics/BOB-16446/14557733

There are also a few parts that are available in a variety of kits and from other sources.  Below are variations that are available via Amazon:

1. A breadboard (only one needed)
2. Jumper wires
3. A 3.3V power supply (or use the 3.3V pin from the micro:bit breakout).

## Wiring

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

Get the raw (un-interpolated) CO₂ concentration in ppm, updated every 5 seconds. Returns `NaN` on error.

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

# Miscellaneous

I develop micro:bit extensions in my spare time to support activities I'm enthusiastic about, like summer camps and science curricula.  You are welcome to become a sponsor of my micro:bit work (one time or recurring payments), which helps offset equipment costs: [https://github.com/sponsors/bsiever](https://github.com/sponsors/bsiever). Any support at all is greatly appreciated!

## Supported targets

for PXT/microbit

<script src="https://makecode.com/gh-pages-embed.js"></script>
<script>makeCodeRender("{{ site.makecode.home_url }}", "{{ site.github.owner_name }}/{{ site.github.repository_name }}");</script>
