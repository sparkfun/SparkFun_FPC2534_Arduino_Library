
![SparkFun Fingerprint Sensor - FPC2534 Pro (Qwiic))](docs/images/gh-banner-2025-ultrasonic-dist.png "SparkFun Fingerprint Sensor")

# SparkFun Fingerprint Sensor - FPC2534 Pro

Arduino Library for the SparkFun Fingerprint Sensor - FPC2534 Pro

![License](https://img.shields.io/github/license/sparkfun/SparkFun_FPC2534_Arduino_Library)
![Release](https://img.shields.io/github/v/release/sparkfun/SparkFun_FPC2534_Arduino_Library)
![Release Date](https://img.shields.io/github/release-date/sparkfun/SparkFun_FPC2534_Arduino_Library)
![Documentation - build](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_FPC2534_Arduino_Library/build-deploy-ghpages.yml?label=doc%20build)
![Compile - Test](https://img.shields.io/github/actions/workflow/status/sparkfun/SparkFun_FPC2534_Arduino_Library/compile-sketch.yml?label=compile%20test)
![GitHub issues](https://img.shields.io/github/issues/sparkfun/SparkFun_FPC2534_Arduino_Library)

The [SparkFun Fingerprint Sensor - FPC2534 Pro]() is a small, highly capable and robust fingerprint sensor that can easily be integrated into virutally any application. Based off the AllKey Biometric System family from Fingerprints Cards (FPC), the FPC2534AP delivers incredible functionality in a small, compact formfactor.

## Functionality

The SparkFun Fingerprint Sensor - FPC2534 Pro is accessable via a variety of interfaces, including I2C and UART, which are supported by this library.

This library provides a message-based, easy to use interface that enables fingerprint biometric authentication and simple finger-based navigation. Specificatlly, the FPC2534AP provides:

- Fingerprint enrollment - adding a fingerprint to the sensor
- Fingerprint template management - managing recorded fingerprints
- Fingerprint matching/indentification for biometric authentication
- Trackpad like functionalality for simple, finger-based navigation
- Application integration via a variety of communication methods

### Functionality not Supported by the Library

Features that the FPC2534 supports, but are not currently implemented by this library include:

- Encrypted communication. When enabled, the communication to/from the FPC2543 is encrypted by a user provided key. Once this key is set in the device, it cannot be changed.
- Reading and writing template data values to/from the sensor.
- USB Interface - while the SparkFun FPC2534 provides a USB-C interface (enabled via jumper settings), this library doesn't support this interface. This mode of communication is primarily used for computer (non-microcontroller) interaction with the device.

If any of these advanced features are desired for use, an implementation can be found within the Fingerprints FPC2543 SDK, which is available on the [Fingerprints Website](https://www.fpc.com/products/documentation/).

## Communication

The operation of the FPC2534AP is performed by a messaging protocol implemented on the device. A client application sends message requests to the sensor and recieves responses to the request made.

To support integration and messaging, the FPC2534AP provides support for four (4) different communication implementations. These are:

- I2C
- UART
- SPI
- USB

The communication method used is selected via a pair of configuration jumpers on the SparkFun Fingerprint Sensor - FPC2534 Pro board. Further information on the use is oulined in the associated Hookup Guide for the SparkFun fingerprint breakout board.

> [!NOTE]
> The I2C (qwiic) interface for the SparkFun Fingerprint Sensor - FPC2534 Pro board is currently only supported on ESP32 and Raspberry RP2 (RP2040, RP2350) boards. The I2C Implemention of the FPC2534 device performs a dynamic payload transmission that is not supported by the Arduino Wire library. Because of this, a custom implementation is provided by this library for the ESP32 and RP2 platforms.

### Additional Connections

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

#### Getting Started

How the sensor is initialized is dependant on the communication method being utilized. The following sections outline how to use setup and initialize the I2C or UART interfaces to the device. Once setup, the operation of the device is communication method independant.

##### Using I2C (Qwiic)

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

###### A note on "pinging" the FPC2534 sensor

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

##### Using UART (Serial)

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
>
> ```c++
>  Serial1.setFIFOSize(512);
>```
>
> On an ESP32:
>
> ```c++
>  Serial.setRxBufferSize(512);
> ```

If using a different controller for your project, the method used to expand the Serial read buffer will need to be determined. Some platforms (STM32 appears to self adjust FIFO size) no additional calls are needed, but on others (normally older systems) no option exists rendering the Serial interface the FPC2543 un-usable on the platform.

To initialize the device, the Serial object used to communicate with the device is passed into the begin call.

An example of calling the begin method:

```c++
    bool status = mySensor.begin(Serial1);
```

At this point, the sensor is ready for normal operation.

#### General Operation

The operational pattern for the SparkFun FPC2543 Fingerprint sensor library is outlined in the following diagram:

![Operational Sequence](docs/images/sfe-fpc2543-op-seq.png)

1) The first step is library initialization and setup.
   - The communication interface being used is provided to the library.
   - The operationa callback functions are registered. These functions are called in reponse to commands sent to the sensor.
2) The next phase normally occurs in the ```loop``` section of system operation.
3) The application makes a request/sends a command to the sensor. These calls are asynchronous, and result in a message being sent to the sensor by the library.
4) During each loop iteration, the library method ```processNextResponse()``` is called. This method will check for new messages and process any messages sent by the sensor.
5) When checking for new messages, the library gets the next message/reponse from the device via the in-use communication bus. This message is parsed, and processed.
6) When a response message is parsed and identified, if the host application has registered a callback for this message type, that callback function is called.
   - The user takes the desired application action in the callback function.

The loop sequence of operation - make a request, check for messages and respond via callback functions continue during the operation of the sensor.

#### Navigation Mode

One of the operating modes of FPC2534 is *Navigation Mode*. Enabled by calling the ```startNavigationMode()``` on the library, the FPC2534 acts like a small touch pad/joystick when in Navigation Mode. It should be noted, the ```startNavigationMode()``` method also takes a parameter that sets the oriengation of the sensor. This is used when determining event type (up, down, left, right).

Events from this mode are communicated to the callback function assigned in the ```on_navigation``` field of the callback function structure that is passed to the library. The following events are supported:

- Swipe left
- Swipe right
- Swipe up
- Swipe down
- Press - a quick press and remove of a finger from the sensor
- Long press - triggered after the finger stays pressed on the sensor for a "long time" (~1 second)

The operation of this mode is outlined in the following diagram:

![Navigation Mode](docs/images/sfe-fpc2543-op-nav.png)

1) Standard setup, with a ```on_navigation()``` callback function provided
2) Once the sensor is running, start navigation mode. The sensor will interpret finger movements as navigation events and send messages when events are detected.
3) Loop mode is entered
4) Messages from the sensor are processed
5) When a navigation event message is identified, the message is parsed and the corresponding navigation event sent to the supplied ```on_navigation()``` callback function.  

To further understand how to use Navigation mode, review the Navigaton examples provided with this library.

- [Navigation using I2C](examples/Example01_NavigationI2C/Example01_NavigationI2C.ino)
- [Navigatoin using Seria](examples/Example03_NavigationUART/Example03_NavigationUART.ino)
