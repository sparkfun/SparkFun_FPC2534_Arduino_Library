/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <Wire.h>

#include "SparkFun_FPC2534.h"

//----------------------------------------------------------------------------------
// Finger Enrollment Example
//
// This sketch demonstrates how to enroll fingerprints using the FPC2534
//
//----------------------------------------------------------------------------------
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
//

// variable used to keep track of the number of enrolled templates on the sensor
uint16_t numberOfTemplates = 0;

// Declare our sensor object
SfeFPC2534UART mySensor;

bool isInitialized = false;
// flag to indicate we need to draw the menu
bool drawTheMenu = false;

//------------------------------------------------------------------------------------
// Simple menu methods
//------------------------------------------------------------------------------------
static void drawMenu()
{
    drawTheMenu = false;

    mySensor.setLED(false);

    Serial.println();
    Serial.println("----------------------------------------------------------------");
    Serial.println(" SparkFun FPC2534 Fingerprint Enrollment Example");
    Serial.println("----------------------------------------------------------------");
    Serial.println();
    Serial.print(" Current number of enrolled templates: ");
    Serial.print(numberOfTemplates);
    Serial.println(" used of 30 maximum");
    Serial.println();
    Serial.println(" Select an option (press the menu number):");
    Serial.println("\t1)  To Enroll a new fingerprint");
    Serial.println("\t2)  Erase all existing fingerprint templates");
    Serial.println("\t3)  Validate a fingerprint");
    Serial.println();

    Serial.print("> ");

    // flush the input/output buffers
    Serial.flush();
    while (Serial.available() > 0)
        Serial.read(); // flush any existing input

    uint8_t chIn;
    while (true)
    {
        if (Serial.available() > 0)
        {
            chIn = Serial.read();
            if (chIn == '1' || chIn == '2' || chIn == '3')
            {
                Serial.println((char)chIn); // echo the character
                break;
            }
            else
                Serial.write(7); // beep - invalid character
        }
        delay(10);
    }
    Serial.println();

    // Action time
    if (chIn == '1')
    {
        // lets enroll an new figure
        Serial.println(" Starting finger enrollment - place finger and remove a finger on the sensor to enroll "
                       "a fingerprint");
        mySensor.setLED(true);
        fpc_id_type_t id;
        id.type = ID_TYPE_GENERATE_NEW;
        id.id = 0;
        fpc_result_t rc = mySensor.requestEnroll(id);
        if (rc != FPC_RESULT_OK)
        {
            Serial.print("[ERROR]\tFailed to start enroll - error: ");
            Serial.println(rc);
        }
        else
            Serial.print("\t samples remaining 12..");
    }
    else if (chIn == '2')
    {
        // Delete all templates - do we have any templates?
        if (numberOfTemplates == 0)
        {
            Serial.println("[INFO]\tNo templates to delete");
            drawTheMenu = true;
        }
        else
        {
            Serial.println(" Deleting all templates on the fingerprint sensor");
            fpc_id_type_t id = {0};
            id.type = ID_TYPE_ALL;
            id.id = 0;
            fpc_result_t rc = mySensor.requestDeleteTemplate(id);
            if (rc != FPC_RESULT_OK)
            {
                Serial.print("[ERROR]\tFailed to delete templates - error: ");
                Serial.println(rc);
            }
            else
                numberOfTemplates = 0;
        }
    }
    else if (chIn == '3')
    {
        // Delete all templates - do we have any templates?
        if (numberOfTemplates == 0)
        {
            Serial.println("[INFO]\tNo templates to validate against");
            drawTheMenu = true;
        }
        else
        {
            Serial.print(" Place a finger on the sensor for validation...");
            fpc_id_type_t id = {0};
            id.type = ID_TYPE_ALL;
            id.id = 0;
            fpc_result_t rc = mySensor.requestIdentify(id, 1);
            if (rc != FPC_RESULT_OK)
            {
                Serial.print("[ERROR]\tFailed to start identity - error: ");
                Serial.println(rc);
            }
        }
    }
    else
        Serial.println("[ERROR]\tInvalid selection");
}

//------------------------------------------------------------------------------------
// Callback functions the library calls
//------------------------------------------------------------------------------------

//----------------------------------------------------------------------------
// on_error()
//
// Call if the sensor library detects/encounters an error
//
static void on_error(uint16_t error)
{
    // hal_set_led_status(HAL_LED_STATUS_ERROR);
    Serial.print("[ERROR]\tSensor Error Code: ");
    Serial.println(error);
    // this could indicated the sensor communications is out of synch - a reset might be needed
    reset_sensor();
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

        // Request the templates on the device ...
        fpc_result_t rc = mySensor.requestListTemplates();
        if (rc != FPC_RESULT_OK)
        {
            Serial.println("[ERROR]\tFailed to get template list - error: ");
            Serial.println(rc);
        }
    }
}

//----------------------------------------------------------------------------
static void on_identify(bool is_match, uint16_t id)
{

    Serial.print(is_match ? " NO " : " ");
    Serial.print("MATCH");
    if (is_match)
    {
        Serial.print("  {Template ID: ");
        Serial.print(id);
        Serial.print("}");
    }
    Serial.println();
}
//----------------------------------------------------------------------------
static void on_enroll(uint8_t feedback, uint8_t samples_remaining)
{

    if (samples_remaining == 0)
    {
        Serial.println("..done!");
        delay(500); // user feedback..
        numberOfTemplates++;
    }
    else
    {
        Serial.print(samples_remaining);
        Serial.print(".");
    }
}
//----------------------------------------------------------------------------
// on_list_templates()
//

static void on_list_templates(uint16_t num_templates, uint16_t *template_ids)
{
    numberOfTemplates = num_templates;

    isInitialized = true;
    // lets draw the menu!
    drawTheMenu = true;
}

//----------------------------------------------------------------------------
// on_status()
static void on_status(uint16_t event, uint16_t state)
{

    // Check the system state, to determine when to draw the menu.
    //
    // The following is checked:
    //
    // 1) End of Enroll or Identify - indicated by:
    //
    //     - EVENT_FINGER_LOST event type
    //     - Sensor is in idle mode
    //
    // 2) Completetion of the Delete All Templates command
    //
    //     - EVENT_NONE event type
    //     - Sensor is in idle mode
    //     - Sensor State is STATE_APP_FW_READY
    //
    //     When this command is completed, a EVENT_NONE event is sent, and the

    if (mySensor.currentMode() == 0)
    {
        // end of enroll or identify
        if (event == EVENT_FINGER_LOST)
            drawTheMenu = true;
        // is the app ready?
        else if ((state & STATE_APP_FW_READY) == STATE_APP_FW_READY)
        {
            // completion of delete all templates
            if (event == EVENT_NONE)
                drawTheMenu = true;
            // the sensor is at idle and has been setup
            else if (event == EVENT_IDLE && isInitialized)
                drawTheMenu = true; // just in
        }
    }
    else if (mySensor.currentMode() == STATE_ENROLL && event == EVENT_FINGER_LOST)
    {
        Serial.print(".");
    }
}

// Define our command callback struct - functions are set in the setup function
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
    Serial.println(" SparkFun FPC2534 Fingerprint Enrollment Example");
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
    cmd_cb.on_enroll = on_enroll;
    cmd_cb.on_identify = on_identify;
    cmd_cb.on_list_templates = on_list_templates;
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
        Serial.print("[ERROR] Processing Error: ");
        Serial.println(rc);
        // Hmm - reset the sensor and start again?
        reset_sensor();
    }
    else if (drawTheMenu)
    {
        drawMenu();
    }

    delay(200);
}