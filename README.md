
![SparkFun Fingerprint Sensor - FPC2534 Pro (Qwiic))](docs/images/gh-banner-2025-ultrasonic-dist.png "SparkFun Fingerprint Sensor")

# SparkFun Fingerprint Sensor - FPC2534 Pro

Arduino Library for the SparkFun Fingerprint Sensor - FPC2534 Pro

![License](https://img.shields.io/github/license/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library)
![Release](https://img.shields.io/github/v/release/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library)
![Release Date](https://img.shields.io/github/release-date/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library)
![Documentation - build](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library/build-deploy-ghpages.yml?label=doc%20build)
![Compile - Test](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library/compile-sketch.yml?label=compile%20test)
![GitHub issues](https://img.shields.io/github/issues/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library)

The [SparkFun Fingerprint Sensor - FPC2534 Pro]() is a small, highly capable and robust fingerprint sensor that can easily be integrated into virutally any application. Based off the AllKey Biometric System family from Fingerprints Cards (FPC), the FPC2534AP delivers incredible functionality in a small, compact formfactor. 

## Functionality

The SparkFun Fingerprint Sensor - FPC2534 Pro is accessable via a variety of interfaces, including I2C and UART, which are supported by this library. 

This library provides a message-based, easy to use interface that enables fingerprint biometric authentication and simple finger-based navigation. Specificatlly, the FPC2534AP provides:

- Fingerprint enrollment - adding a fingerprint to the sensor 
- Fingerprint template management - managing recorded fingerprints
- Fingerprint matching/indentification for biometric authentication
- Trackpad like functionalality for simple, finger-based navigation
- Application integration via a variety of communication methods

### Communication

The operation of the FPC2534AP is performed by a messaging protocol implemented on the device. A client application sends message requests to the sensor and recieves responses to the request made. 

To support integration and messaging, the FPC2534AP provides support for four (4) different communication implementations. These are:

- I2C
- UART
- SPI
- USB

The communication method used is selected via a pair of configuration jumpers on the SparkFun Fingerprint Sensor - FPC2534 Pro board. Further information on the use is oulined in the associated Hookup Guide for the SparkFun fingerprint breakout board.

> [!NOTE]
> The I2C (qwiic) interface for the SparkFun Fingerprint Sensor - FPC2534 Pro board is currently only supported on ESP32 and Raspberry RP2 (RP2040, RP2350) boards. The I2C Implemention of the FPC2534 device performs a dynamic payload transmission that is not supported by the Arduino Wire library. Because of this, a custom implementation is provided by this library for the ESP32 and RP2 platforms.

#### Additional Connections

In addition to the communication method selected, the FPC2534AP requires additional connections to facilitate communication. The following connections are required:

| Pin | Use | Notes |
| -- | -- | -- |
|RST | Reset Device | Setting the pin low, then high will reset the FPC2534AP, placing it into a known state|
|IQ| IRQ/Interrupt Request|Used by the sensor to signal a new message/data availablity - note: this is not required when using the serial interface|

## Library

Unlike a majority of sensors and their associated libraries which are synchronous function calls - a function/method call returns a requested value or performs a specific action - the FPC2534AP operates using a messaging methodlogy - sending messages to the client as actions occur on the device, or in reponse to earlier requests. 

Since messaging methodology is used by the FPC2534AP, this library makes makese use of the following implementation pattern:

- User provided callback functions that the library calls when a specific message is recieved from the library (like when a finger is pressed on the sensor).
- A process message method that is called to process the next available message from the sensor if one is available. This method is called within your applications main processing loop (for Arduino, this is the `loop()` function).

While this methodlogy is unique to the when compaired to other libraries, it fits well with the event driven nature of the FPC2534AP fingerprint sensor. 

### Using the Library

The first step to using the library is selected the method used to communicate with the device. The library supports I2C on select platforms, or UART (a Serial interface in Arduino). Once selected, and device connected as outlined in the hookup guide for theSparkFun Fingerprint Sensor - FPC2534 Pro. The type of connection depends on the method used to communicate with the device. 

#### Using I2C (Qwiic)

When using I2C to communicate with the fingerprint sensor, the class named `SfeFPC2534I2C` is used. An example of how to declare the sensor object is as follows:

```c++
// Declare our sensor object
SfeFPC2534I2C mySensor;
```

To initialize the device, the following is needed:

- The address of the device
- The Arduino Wire object being used for the device
- The number for the I2C bus being used (this is needed to support I2C read operations from the sensor) - Normally this is a value of `0` or `1`
- The Pin number the IRQ of the device is connected to. This IRQ is used by the sensor to indicated data is available for reading from the device.

An example of calling the begin method:

```c++
    bool status =  mySensor.begin(SFE_FPC2534_I2C_ADDRESS, Wire, i2cBusNumber, interruptPin );
```

At this point, the sensor is ready for normal operation.

##### A note on "pinging" the FPC2534 sensor 

Often, to determine if a sensor is available on the I2C bus, the bus is queried at the address for the device  (a simple "ping"). In arduino this often looks like:

```c++
// Is the sensor there - on the I2C bus?
    Wire.beginTransmission(kFPC2534DefaultAddress);
    if (Wire.endTransmission() == 0)
        Serial.println("The Touch Sensor FPC2534 is available");
```

Developing with the sensor has shown that once the sensor is "pinged", it enters an unknown state. To ensure proper device operation, a `reset()` of the device is needed after a `ping` operation is performed. 

> [!NOTE]
> The I2C (qwiic) interface for the SparkFun Fingerprint Sensor - FPC2534 Pro board is currently only supported on ESP32 and Raspberry RP2 (RP2040, RP2350) boards. The I2C Implemention of the FPC2534 device performs a dynamic payload transmission that is not supported by the Arduino Wire library. Because of this, a custom implementation is provided by this library for the ESP32 and RP2 platforms.

#### Using UART (Serial)

When using a UART (Serial) to communicate with the fingerprint sensor, the class named `SfeFPC2534UART` is used. An example of how to declare the sensor object is as follows:

```c++
// Declare our sensor object
SfeFPC2534UART mySensor;
```

Configure the following settings on the UART/Serial connection being used:

| Setting | Value |
| -- | -- |
| Baud Rate | 921600 |
| Config | SERIAL_8N1|
| Read Buffer Size | 512|


> [!NOTE]
> Due to the amount of information sent by the fingerprint sensor, the default size of the  internal buffer used by Arduino Serial objects is rapidly exceeded. To prevent this, the buffer size must be be increased before intializing the FPC2534AP device.  
> 
> To increase the buffer size when using a Raspberry pi RP2 Microcontroller:
> ```c++
>  Serial1.setFIFOSize(512);
>```
> On an ESP32:
> ```c++
>  Serial.setRxBufferSize(512);
> ```


To initialize the device, the Serial object used to communicate with the device is passed into the begin call. 


An example of calling the begin method:

```c++
    bool status = mySensor.begin(Serial1);
```

At this point, the sensor is ready for normal operation.

