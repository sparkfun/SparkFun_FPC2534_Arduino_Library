
![SparkFun Fingerprint Sensor - FPC2534 Pro (Qwiic))](docs/images/gh-banner-2025-ultrasonic-dist.png "SparkFun Fingerprint Sensor")

# SparkFun Fingerprint Sensor - FPC2534 Pro

Arduino Library for the SparkFun Fingerprint Sensor - FPC2534 Pro

![License](https://img.shields.io/github/license/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library)
![Release](https://img.shields.io/github/v/release/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library)
![Release Date](https://img.shields.io/github/release-date/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library)
![Documentation - build](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library/build-deploy-ghpages.yml?label=doc%20build)
![Compile - Test](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library/compile-sketch.yml?label=compile%20test)
![GitHub issues](https://img.shields.io/github/issues/sparkfun/SparkFun_Qwiic_Ultrasonic_Arduino_Library)

The [SparkFun Fingerprint Sensor - FPC2534 Pro]()is a small, highly capable and robust fingerprint sensor that can easily be integrated into virutally any application. Based off the AllKey Biometric System family from Fingerprints Cards (FPC), the FPC2534AP delivers incredible functionality in a small, compact formfactor. 

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

The first step to using the library is selected the method used to communicate with the device. The library supports I2C on select platforms, or UART (a Serial interface in Arduino)
