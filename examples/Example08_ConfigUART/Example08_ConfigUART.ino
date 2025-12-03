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
 * Example using the SparkFun FPC2534 Fingerprint sensor library to demonstrate the configuration
 * capabilities of the sensor.  The example retrieves the current system configuration from the sensor,
 * modifies one of the configuration values, and sends the updated configuration back to the sensor.
 *
 * NOTE: Currently, the set of the updated values fails.
 *
 * This version of this example uses the SPI interface to communicate with the sensor.
 *
 *---------------------------------------------------------------------------------
 */

#include <Arduino.h>

#include "SparkFun_FPC2534.h"

//----------------------------------------------------------------------------
// User Config -
//----------------------------------------------------------------------------
// Define the pins being used for operation
//
// The FPC2534 uses one pin for reset, No interrupt pin is needed for UART operation
//
// The following pin definitions were used for testing - but can be modified as needed.
//
// ESP32 thing plus
// #define RST_PIN 21

// ESP32 thing plus C
// #define RST_PIN 14

// RP2350 thing plus
#define RST_PIN 12

// IoT RedBoard - RP2350
// #define RST_PIN 29

//----------------------------------------------------------------------------------
// NOTE:
// This example makes use of the Serial1 hardware serial port for communication with the FPC2534. If the board
// being used does not have a Serial1 port, you will need to modify the code to use SoftwareSerial or another
// serial port available on your board.

// Declare our sensor object. Note the SPI version of the sensor class is used.
SfeFPC2534UART mySensor;

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
    deviceIdle = (event == EVENT_IDLE);
}

//----------------------------------------------------------------------------
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
    Serial.print("  Allows Factory Reset:\t\t  ");
    Serial.println((cfg.sys_flags & CFG_SYS_FLAG_ALLOW_FACTORY_RESET) ? "Yes" : "No");
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

    // result of our initial request to get the config
    if (configState == CONFIG_REQUESTED)
    {
        // stash our current config as the default
        the_config = *cfg;

        configState = CONFIG_RECEIVED;
    }
    else if (configState == CONFIG_VERIFY)
    {
        // verify the config set worked
        if (cfg->idfy_max_consecutive_fails == the_config.idfy_max_consecutive_fails + 1)
            Serial.println("[INFO]\t\tConfiguration update verified successfully.");
        else
        {
            Serial.println();
            Serial.println("[ERROR]\tConfiguration update verification failed.");
            Serial.println();
        }

        // back to entry state
        configState = CONFIG_RESET;
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
// Process the simple sequece of steps for the demo.

void process_demo_steps(void)
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
    Serial.println();
    Serial.println(" **NOTE** Currently the `setSystemConfig` command fails to set the updated values.");
    Serial.println();
    Serial.println("----------------------------------------------------------------");
    Serial.println();

    // The internal UART buffer can fill up quickly and overflow. As such, increase its size.
    // This example is supporting ESP32 and RP2040 based boards - adjust as needed for other platforms.
#if defined(ARDUINO_ARCH_RP2040)
    Serial1.setFIFOSize(512);
#elif defined(ESP32)
    Serial1.setRxBufferSize(512);
#endif

    Serial1.begin(921600, SERIAL_8N1);
    delay(100);
    for (uint32_t startMS = millis(); !Serial1 && (millis() - startMS < 5000);) // Wait for the serial port to be ready
        delay(200);
    Serial.println("[STARTUP]\tSerial1 started for FPC2534 communication.");

    // Reset the sensor to ensure it's in a known state - by default this also triggers the
    // sensor to send a status message
    reset_sensor();

    // Initialize the sensor library
    if (!mySensor.begin(Serial1))
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

    // Reset the sensor to start fresh
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
        process_demo_steps();

    delay(200);
}