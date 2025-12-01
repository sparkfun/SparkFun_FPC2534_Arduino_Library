/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

/*
 * Example using the SparkFun FPC2534 Fingerprint sensor library to demonstrate the fingerprint
 * enrollment and identification of the sensor.  The example provides the user with the following options:
 *    - Enroll a new fingerprint
 *    - Delete all existing fingerprints
 *    - Validate a fingerprint
 *
 * This version of this example uses the SPI interface to communicate with the sensor.
 *
 * Example Setup:
 *  - Connect the SparkFun Qwiic FPC2534 Fingerprint sensor to your microcontroller using hook-up wires ...
 *     setup for SPI communication. Note the CS PIN number - this needs to be defined below.
 *  - Connect the RST pin on the sensor to a digital pin on your microcontroller. This is used by the
 *    example to "reset the sensor" on startup.
 *  - Connect the IRQ pin on the sensor to a digital pin on your microcontroller. The sensor triggers
 *    an interrupt on this pin when it has data to send.
 *  - Update the IRQ_PIN and RST_PIN defines below to match the pins you are using.
 *
 * Operation:
 *  - The sensor object is initialized with the CS pin and the interrupt pin. This examples uses the default SPI bus.
 *  - The example registers several callback functions with the sensor library. These functions are called as
 *    messages are received from the sensor.
 *  - Once running, the example prevents a menu with the following options:
 *      1) Enroll a new fingerprint
 *      2) Erase all existing fingerprint templates
 *      3) Validate a fingerprint
 *
 *   Once an optoin is selected (enter the menu number), the example performs the requested operation.
 *
 *  - When registering a new fingerprint, the example prompts the user to place and remove their finger repeatedly
 *    until the fingerprint is fully enrolled. The example prints out the number of samples remaining as
 *    reported by the sensor. Normally 12 samples are needed to fully enroll a fingerprint.
 *  - When deleting all fingerprints, the example sends the delete command to the sensor. If successful,
 *    the example resets its internal count of enrolled fingerprints to zero.
 *  - When validating a fingerprint, the example prompts the user to place their finger on the sensor.
 *    If the fingerprint matches an enrolled fingerprint, the example prints out the ID of the matched
 *    fingerprint template.
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
// These are the pins the CS, IRQ and RST pins of the sensor are connected to the microcontroller.
//
// NOTE: The IRQ pin must be an interrupt-capable pin on your microcontroller
//
// Example pins for various SparkFun boards:
//
// ESP32 thing plus
// #define IRQ_PIN 16
// #define RST_PIN 21
// #define CS_PIN

// ESP32 thing plus C
// #define IRQ_PIN 32
// #define RST_PIN 14
// #define CS_PIN

// ESP32 IoT RedBoard
// #define IRQ_PIN 26
// #define RST_PIN 27
// #define CS_PIN 25

// rp2350 thing plus
// #define IRQ_PIN 11
// #define RST_PIN 12
// #define CS_PIN

// rp2350 RedBoard IoT
#define IRQ_PIN 29
#define RST_PIN 28
#define CS_PIN 21

// Declare our sensor object. Note the SPI version of the sensor class is used.
SfeFPC2534SPI mySensor;

// flag used to indicate if config has been set....
#define CONFIG_BEGIN 0
#define CONFIG_REQUESTED 1
#define CONFIG_RECEIVED 2
#define CONFIG_SET 3
#define CONFIG_VERIFY 4
#define CONFIG_RESET 5
#define CONFIG_COMPLETE 6
#define CONFIG_EXIT 255

uint8_t configState = CONFIG_BEGIN;

fpc_system_config_t the_config = {0};
bool deviceIdle = false;
//------------------------------------------------------------------------------------
// Callback functions the library calls
//------------------------------------------------------------------------------------

//----------------------------------------------------------------------------
// on_error()
//
// Called if the sensor library detects/encounters an error
//
static void on_error(uint16_t error)
{
    Serial.print("[ERROR]\tSensor Error Code: ");
    Serial.println(error);

    // this could indicated the sensor communications is out of synch - a reset might be needed
    reset_sensor();
}

//----------------------------------------------------------------------------
// on_is_ready_change()
//
// Called when the device ready state changes
//
static void on_is_ready_change(bool isReady)
{
    // On startup the device isn't immediately ready. A message is sent when it is.
    // The Library will call this function when that happens

    if (isReady)
    {
        Serial.println("[STARTUP]\tFPC2534 Device is ready");

        Serial.println("[INFO]\t\tRequesting system configuration...");
        // Request the configuration from the connected device.
        fpc_result_t rc = mySensor.requestGetSystemConfig(FPC_SYS_CFG_TYPE_DEFAULT);
        if (rc != FPC_RESULT_OK)
        {
            Serial.print("[ERROR]\tGet config request failed - error: ");
            Serial.println(rc);
        }
        else
            configState = CONFIG_REQUESTED;
    }
}

//----------------------------------------------------------------------------
// on_status()
//
// Called when the sensor sends a status message
//
static void on_status(uint16_t event, uint16_t state)
{
    Serial.print("[STATUS]\tEvent: 0x");
    Serial.print(event, HEX);
    Serial.print(" State: 0x");
    Serial.println(state, HEX);

    deviceIdle = (event == EVENT_IDLE);
}
static void update_config(fpc_system_config_t &new_config)
{
    // The current values haven't been received yet
    if (configState < CONFIG_RECEIVED)
        return;

    fpc_result_t rc = mySensor.setSystemConfig(&new_config);
    if (rc != FPC_RESULT_OK)
    {
        Serial.print("[ERROR]\tSet config request failed - error: ");
        Serial.println(rc);
    }
}
//----------------------------------------------------------------------------
static void update_config_max_fails(uint8_t max_fails)
{
    // The current values haven't been received yet
    if (configState < CONFIG_RECEIVED)
        return;
    // copy over the current defaults
    fpc_system_config_t new_config = the_config;

    // update the value of max consecutive fails
    new_config.idfy_max_consecutive_fails = max_fails;

    Serial.println();
    delay(500);
    Serial.print("[INFO]\t\tSetting Max Fails To: ");

    Serial.println(new_config.idfy_max_consecutive_fails);

    update_config(new_config);
}
//----------------------------------------------------------------------------
static void print_config(fpc_system_config_t &cfg)
{

    Serial.print("  Version:\t\t\t  ");
    Serial.println(cfg.version);
    Serial.print("  Finger Scan Interval (ms):\t  ");
    Serial.println(cfg.finger_scan_interval_ms);
    Serial.print("  System Flags:\t\t\t  0x");
    Serial.println(cfg.sys_flags, HEX);
    Serial.print("  UART Delay Before IRQ (ms):\t  ");
    Serial.println(cfg.uart_delay_before_irq_ms);
    Serial.print("  UART Baudrate:\t\t  0x");
    Serial.println(cfg.uart_baudrate, HEX);
    Serial.print("  Max Consecutive Identify Fails: ");
    Serial.println(cfg.idfy_max_consecutive_fails);
    Serial.print("  Identify Lockout Time (s):\t  ");
    Serial.println(cfg.idfy_lockout_time_s);
    Serial.print("  Idle Time Before Sleep (ms):\t  ");
    Serial.println(cfg.idle_time_before_sleep_ms);
    Serial.print("  Enroll Touches:\t\t  ");
    Serial.println(cfg.enroll_touches);
    Serial.print("  Enroll Immobile Touches:\t  ");
    Serial.println(cfg.enroll_immobile_touches);
    Serial.print("  I2C Address (7bit):\t\t  0x");
    Serial.println(cfg.i2c_address, HEX);
}
//----------------------------------------------------------------------------
// on_system_config_get()
void on_system_config_get(fpc_system_config_t *cfg)
{
    if (cfg == NULL)
    {
        Serial.println("[ERROR]\tInvalid configuration data received");
        return;
    }
    // first print the config
    Serial.println("[CONFIG]\tSystem Configuration Received:");
    print_config(*cfg);

    if (configState == CONFIG_REQUESTED)
    {
        // stash our current config as the default
        the_config = *cfg;

        configState = CONFIG_RECEIVED;
    }
    else if (configState == CONFIG_VERIFY)
    {
        // verify the set worked
        if (cfg->idfy_max_consecutive_fails == the_config.idfy_max_consecutive_fails + 1)
        {
            Serial.println("[INFO]\t\tConfiguration update verified successfully.");
            configState = CONFIG_RESET;
        }
        else
        {
            Serial.println("[ERROR]\tConfiguration update verification failed.");
        }
    }
}
//------------------------------------------------------------------------------------
// Callbacks
//
// Define our command callbacks structure - callback methods are assigned in setup
static sfDevFPC2534Callbacks_t cmd_cb = {0};

//------------------------------------------------------------------------------------
// reset_sensor()
//
// Simple function to toggle the reset pin of the sensor
//
void reset_sensor(void)
{
    // Reset the sensor by toggling the reset pin.
    //
    // clear out our data buffer
    mySensor.clearData();
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, LOW); // Set reset pin low
    delay(10);                  // Wait for 10 ms

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
    Serial.println(" SparkFun FPC2534 Fingerprint Config Example - SPI");
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

    // setup our callback functions structure
    cmd_cb.on_error = on_error;
    cmd_cb.on_status = on_status;
    cmd_cb.on_is_ready_change = on_is_ready_change;
    cmd_cb.on_system_config_get = on_system_config_get;

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
        Serial.print("[ERROR] Processing Error: ");
        Serial.println(rc);
        // Hmm - reset the sensor and start again?
        reset_sensor();
    }
    else if (deviceIdle)
    {
        // if we have the config, and haven't updated it yet, do so now
        if (configState == CONFIG_RECEIVED)
        {
            // update the max fails value
            update_config_max_fails(the_config.idfy_max_consecutive_fails + 1);
            configState = CONFIG_SET;
        }
        else if (configState == CONFIG_SET)
        {
            // Get the new value of the config to verify the set worked
            Serial.println("[INFO]\t\tRequesting system configuration...");
            // Request the configuration from the connected device.
            fpc_result_t rc = mySensor.requestGetSystemConfig(FPC_SYS_CFG_TYPE_DEFAULT);
            if (rc != FPC_RESULT_OK)
            {
                Serial.print("[ERROR]\tGet config request failed - error: ");
                Serial.println(rc);
            }
            else
                configState = CONFIG_VERIFY;
        }
        else if (configState == CONFIG_RESET)
        {
            // Reset to defaults
            fpc_result_t rc = mySensor.setSystemConfig(&the_config);
            if (rc != FPC_RESULT_OK)
            {
                Serial.print("[ERROR]\tSet config request failed - error: ");
                Serial.println(rc);
            }
            else
                Serial.println("[INFO]\t\tSet config to the original value");
            configState = CONFIG_COMPLETE;
        }
        else if (configState == CONFIG_COMPLETE)
        {
            // Get the new value of the config to verify the set worked
            Serial.println("[INFO]\t\tRequesting final configuration...");
            // Request the configuration from the connected device.
            fpc_result_t rc = mySensor.requestGetSystemConfig(FPC_SYS_CFG_TYPE_DEFAULT);
            if (rc != FPC_RESULT_OK)
            {
                Serial.print("[ERROR]\tGet config request failed - error: ");
                Serial.println(rc);
            }
            else
                configState = CONFIG_EXIT;
        }
    }

    delay(200);
}