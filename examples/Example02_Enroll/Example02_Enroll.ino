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
// The FPC2534 uses one pin for reset, and for I2C requires an interrupt pin to signal
// when data is available to read.
//
// The following pin definitions were used for testing - but can be modified as needed.
//
// ESP32 thing plus
// #define IRQ_PIN 16
// #define RST_PIN 21

// ESP32 thing plus C
// #define IRQ_PIN 32
// #define RST_PIN 14

// RP2350 thing plus
#define IRQ_PIN 11
#define RST_PIN 12

// IoT RedBoard - RP2350
// #define IRQ_PIN 28
// #define RST_PIN 29

// TODO: WTF on the uart / serial layer
// #define UART_RX 32
// #define UART_TX 14

uint16_t numberOfTemplates = 0;

// Declare our sensor object
SfeFPC2534I2C mySensor;

bool isInitialized = false;
// flag to indicate we need to draw the menu
bool drawTheMenu = false;

//------------------------------------------------------------------------------------
// Simple menu methods
//------------------------------------------------------------------------------------
static void drawMenu()
{
    drawTheMenu = false;

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
        Serial.println(
            "[INFO]\tStarting finger enrollment process - place finger on sensor and remove when progress is reported");
        mySensor.setLED(true);
        fpc_id_type_t id = {.type = ID_TYPE_GENERATE_NEW, .id = 0};
        fpc_result_t rc = mySensor.requestEnroll(id);
        if (rc != FPC_RESULT_OK)
            Serial.printf("[ERROR]\tFailed to start enroll - error: %d\n\r", rc);
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
            Serial.println("[INFO]\tDeleting all templates on the fingerprint sensor");
            fpc_id_type_t id = {.type = ID_TYPE_ALL, .id = 0};
            fpc_result_t rc = mySensor.requestDeleteTemplate(id);
            if (rc != FPC_RESULT_OK)
                Serial.printf("[ERROR]\tFailed to delete templates - error: %d\n\r", rc);
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
            Serial.println("[INFO]\t Place a finger on the sensor for validation");
            fpc_id_type_t id = {.type = ID_TYPE_ALL, .id = 0};
            fpc_result_t rc = mySensor.requestIdentify(id, 1);
            if (rc != FPC_RESULT_OK)
                Serial.printf("[ERROR]\tFailed to start identity - error: %d\n\r", rc);
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
    Serial.printf("[ERROR]\tSensor Error Code: %d\n\r", error);
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
            Serial.printf("[ERROR]\tFailed to get template list - error: %d\n\r", rc);
    }
}

//----------------------------------------------------------------------------
static void on_identify(bool is_match, uint16_t id)
{
    Serial.printf("[INFO]\t\tIdentify Result: %s\n\r", is_match ? "MATCH" : "NO MATCH");
    if (is_match)
        Serial.printf("\t\tMatched Template ID: %d\n\r", id);
}
//----------------------------------------------------------------------------
static void on_enroll(uint8_t feedback, uint8_t samples_remaining)
{

    Serial.printf("[INFO]\t\tEnroll samples remaining: %d, feedback: %s (%d)\n\r", samples_remaining,
                  mySensor.getEnrollFeedBackString(feedback), feedback);

    if (samples_remaining == 0)
    {
        Serial.println("[INFO]\tEnroll complete");
        drawTheMenu = true;
        numberOfTemplates++;
    }
}
//----------------------------------------------------------------------------
// on_list_templates()
//

static void on_list_templates(uint16_t num_templates, uint16_t *template_ids)
{
    numberOfTemplates = num_templates;
    // Serial.printf("[INFO]\t\tNumber of templates on the sensor: %d\n\r", num_templates);

    isInitialized = true;
    // lets draw the menu!
    drawTheMenu = true;
}

//----------------------------------------------------------------------------
// on_status()
static void on_status(uint16_t event, uint16_t state)
{
    Serial.printf("[STATUS]\tEvent: 0x%04X, State: 0x%04X\n\r", event, state);

    // Check the system state, to determine when to draw the menu.
    //
    // The following is checked:
    //
    // 1) End of Enroll or Identify - indicated by:
    //
    //     - EVENT_FINGER_LOST event type
    //     - Sensor is in idel mode
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

    // if checking identity and we get an image ready event - the device hangs until finger up
    // Let's prompt the user to remove the finger
    else if (mySensor.currentMode() == STATE_IDENTIFY && event == EVENT_IMAGE_READY)
    {
        Serial.println("[INFO]\t\tUnable to perform ID check - remove finger and try again");
    }
}

// Define our command callbacks the library will call on events from the sensor
static const sfDevFPC2534Callbacks_t cmd_cb = {.on_error = on_error,
                                               .on_status = on_status,
                                               .on_enroll = on_enroll,
                                               .on_identify = on_identify,
                                               .on_list_templates = on_list_templates,
                                               .on_is_ready_change = on_is_ready_change};

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

    // Initialize the I2C communication
    Wire.begin();

    // Reset the sensor to ensure it's in a known state - by default this also triggers the
    // sensor to send a status message
    reset_sensor();

    // Is the sensor there - on the I2C bus?
    Wire.beginTransmission(kFPC2534DefaultAddress);
    if (Wire.endTransmission() != 0)
    {
        Serial.println("[ERROR]\tTouch Sensor FPC2534 not found on I2C bus. HALT");
        while (1)
        {
            delay(1000); // Wait indefinitely if device is not found
        }
    }
    else
        Serial.println("[STARTUP]\tTouch Sensor FPC2534 found on I2C bus");

    // Initialize the sensor library
    if (!mySensor.begin(kFPC2534DefaultAddress, Wire, 0, IRQ_PIN))
    {
        Serial.println("[ERROR]\tFPC2534 not found. Check wiring. HALT.");
        while (1)
            delay(1000);
    }
    Serial.println("[STARTUP]\tFPC2534 initialized.");

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
        Serial.printf("[ERROR] Processing Error: %d\n\r", rc);
        // Hmm - reset the sensor and start again?
        reset_sensor();
    }
    else if (drawTheMenu)
    {
        drawMenu();
    }

    delay(200);
}