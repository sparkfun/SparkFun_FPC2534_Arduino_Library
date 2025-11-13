
/*
 * ---------------------------------------------------------------------------------
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 * ---------------------------------------------------------------------------------
 */

/*
 * Example using the SparkFun FPC2534 Fingerprint sensor library to demonstrate navigation mode
 * of the sensor. This example uses the SPI interface to communicate with the sensor.
 *
 * Example Setup:
 *   - Connect the SparkFun Qwiic FPC2534 Fingerprint sensor to your microcontroller using SPI.
 *  - Connect the RST pin on the sensor to a digital pin on your microcontroller. This is used by the
 *    example to "reset the sensor" on startup.
 *  - Connect the IRQ pin on the sensor to a digital pin on your microcontroller. The sensor triggers
 *    an interrupt on this pin when it has data to send.
 *  - Update the IRQ_PIN and RST_PIN defines below to match the pins you are using.
 *
 * Operation:

 * - The example registers several callback functions with the sensor library. These functions are called as
 *   messages are received from the sensor.
 * - The example places the sensor in navigation mode. In this mode, the sensor detects simple gestures
 *   such as left, right, up, down swipes, as well as press and long-press events.
 * - The example prints out messages to the serial console as events are received from the sensor.
 * - On a press event, the example toggles the on-board LED of the sensor on and off.
 * - On a long-press event, the example requests the firmware version from the sensor and prints it out when received.
 *

 *---------------------------------------------------------------------------------
 */
#include <Arduino.h>

#include "SparkFun_FPC2534.h"

//----------------------------------------------------------------------------
// User Config -
//----------------------------------------------------------------------------
// UPDATE THESE DEFINES TO MATCH YOUR HARDWARE SETUP
//
// These are the pins the IRQ and RST pins of the sensor are connected to the microcontroller.
//
// NOTE: The IRQ pin must be an interrupt-capable pin on your microcontroller
//
// Example pins tested for various SparkFun boards:

// // ESP32 thing plus
// #define IRQ_PIN 16
// #define RST_PIN 21
// #define I2C_BUS 0

// ESP32 thing plus C
// #define IRQ_PIN 32
// #define RST_PIN 14
// #define I2C_BUS 0

// ESP32 IoT RedBoard
#define IRQ_PIN 26
#define RST_PIN 27
#define CS_PIN 6

// rp2350 thing plus
// #define IRQ_PIN 11
// #define RST_PIN 12
// #define I2C_BUS 0

// rp2350 RedBoard IoT
// #define IRQ_PIN 29
// #define RST_PIN 28
// #define I2C_BUS 0

// State flags to manage sensor startup/state
bool startNavigation = true;

// Used to track LED state
bool ledState = false;

// Declare our sensor object. Note the SPI version of the sensor class is used.
SfeFPC2534SPI mySensor;

//------------------------------------------------------------------------------------
// Callback functions the library calls
//------------------------------------------------------------------------------------
// Unlike a majority of Arduino Sensor libraries, the FPC2534 library is event/callback driven.
// The library calls functions you define when events occur. This allows your code to respond
// to messages from the sensor. These messages are ready by calling the processNextResponse() function
// in your main loop.
//----------------------------------------------------------------------------
// on_error()
//
// Call if the sensor library detects/encounters an error
//
static void on_error(uint16_t error)
{
    // Just print the error code
    Serial.print("[ERROR] code:\t");
    Serial.println(error);
}

//----------------------------------------------------------------------------
// on_is_ready_change()
//
// Call when the device ready state changes
//
static void on_is_ready_change(bool isReady)
{
    // On startup the device isn't immediately ready. A message is sent when it is.
    // The Library will call this function when that happens

    if (isReady)
    {
        Serial.println("[STARTUP]\tFPC2534 Device is ready");

        // do we need to start navigation mode?
        if (mySensor.currentMode() != STATE_NAVIGATION && startNavigation)
        {
            // Place the sensor in Navigation mode and print out a menue.
            startNavigation = false;
            fpc_result_t rc = mySensor.startNavigationMode(0);

            // error?
            if (rc != FPC_RESULT_OK)
            {
                Serial.print("[ERROR]\tFailed to start navigation mode - error:");
                Serial.println(rc);
                return;
            }

            Serial.println("[SETUP]\tSensor In Navigation mode.");
            Serial.println();
            Serial.println("\t- Swipe Up, Down, Left, Right to see events.");
            Serial.println("\t- Press to toggle LED on/off.");
            Serial.println("\t- Long Press to get firmware version.");
            Serial.println();
        }
        else
            Serial.println("[STATUS] \tFPC2534 Device is NOT ready");
    }
}

//----------------------------------------------------------------------------
// on_version()
//
// Call when the sensor sends a version string
//
static void on_version(char *version)
{
    // just print the version string
    Serial.print("\t\t");
    Serial.println(version);
}

//----------------------------------------------------------------------------
// on_navigation()
//
// Call when the sensor sends a navigation event
//
static void on_navigation(uint16_t gesture)
{
    Serial.print("[NAVIGATION]\t");
    switch (gesture)
    {
    case CMD_NAV_EVENT_NONE:
        Serial.println("NONE");
        break;
    case CMD_NAV_EVENT_UP:
        Serial.println("UP");
        break;
    case CMD_NAV_EVENT_DOWN:
        Serial.println("DOWN");
        break;
    case CMD_NAV_EVENT_RIGHT:
        Serial.println("RIGHT");
        break;
    case CMD_NAV_EVENT_LEFT:
        Serial.println("LEFT");
        break;
    case CMD_NAV_EVENT_PRESS:
        // Toggle the on-board  LED
        Serial.print("PRESS -> {LED ");
        Serial.print(ledState ? "OFF" : "ON");
        Serial.println("}");
        ledState = !ledState;
        mySensor.setLED(ledState);
        break;

    case CMD_NAV_EVENT_LONG_PRESS:
        // Request the firmware version from the sensor. The sensor will respond
        // with a version event that will call our on_version() function above.
        Serial.println("LONG PRESS -> {Get Version}");
        mySensor.requestVersion();
        break;
    default:
        Serial.println("UNKNOWN");
        break;
    }
}

// ------------------------------------------------------------------------------------
// Fill in the library callback structure with  our callback functions
//
// This is passed to the library so it knows what functions to call when events occur.
static sfDevFPC2534Callbacks_t cmd_cb = {0};

//------------------------------------------------------------------------------------
// reset_sensor()
//
// Simple function to toggle the reset pin of the sensor
//
void reset_sensor(void)
{
    // Reset the sensor by toggling the reset pin
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, LOW);  // Set reset pin low
    delay(10);                   // Wait for 10 ms
    digitalWrite(RST_PIN, HIGH); // Set reset pin high
    delay(250);                  // Wait for sensor to initialize
}

//------------------------------------------------------------------------------------
// setup()
//
void setup()
{
    delay(2000);

    // Set up serial communication for debugging
    Serial.begin(115200); // Set baud rate to 115200
    while (!Serial)
    {
        ; // Wait for serial port to connect. Needed for native USB port only
    }
    Serial.println();
    Serial.println("----------------------------------------------------------------");
    Serial.println(" SparkFun FPC2534 Navigation Example - I2C");
    Serial.println("----------------------------------------------------------------");
    Serial.println();

    // Configure the CS ping
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH); // Set CS pin high as a start off point.

    // Initialize the SPI communication
    SPI.begin();

    // The sensor is available - Initialize the sensor library
    if (!mySensor.begin(CS_PIN, IRQ_PIN))
    {
        Serial.println("[ERROR]\tFPC2534 not found. Check wiring. HALT.");
        while (1)
            delay(1000);
    }
    Serial.println("[STARTUP]\tFPC2534 initialized.");

    // Setup our callback functions structure
    cmd_cb.on_error = on_error;
    cmd_cb.on_version = on_version;
    cmd_cb.on_navigation = on_navigation;
    cmd_cb.on_is_ready_change = on_is_ready_change;

    // set the callbacks for the sensor library to call
    mySensor.setCallbacks(cmd_cb);

    // One last reset of the sensor = observation shows that this is needed after the above device ping...
    reset_sensor();

    // Ready to go!
    Serial.println("[STARTUP]\tFingerprint system initialized.");
}

//------------------------------------------------------------------------------------
void loop()
{

    // Call the library to process the next response from the sensor. The library will call our above
    // callback functions as events occur.
    fpc_result_t rc = mySensor.processNextResponse();
    if (rc != FPC_RESULT_OK && rc != FPC_PENDING_OPERATION)
    {
        Serial.print("[ERROR] Sensor Processing Error: ");
        Serial.println(rc);
    }

    delay(200);
}