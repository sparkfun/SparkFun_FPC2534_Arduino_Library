

#include <Arduino.h>
#include <Wire.h>

#include "SparkFun_FPC2534.h"

// esp32 thing plus
// #define IRQ_PIN 16
// #define RST_PIN 21

// esp32 thing plus C
// #define IRQ_PIN 32
// #define RST_PIN 14

// rp2350 thing plus
#define IRQ_PIN 11
#define RST_PIN 12

// #define UART_RX 32
// #define UART_TX 14

uint16_t numberOfTemplates = 0;

// Declare our sensor object
SfeFPC2534I2C mySensor;

static void check_sensor_status(void)
{
    // if the sensor is in an "idle", mode, launch navigation mode
    if (mySensor.currentMode() == 0 && mySensor.isReady())
    {
        mySensor.setLED(false);
        fpc_result_t rc = mySensor.startNavigationMode(0);

        // error?
        if (rc != FPC_RESULT_OK)
        {
            Serial.printf("[ERROR]\tFailed to start navigation mode - error: %d\n\r", rc);
            return;
        }

        Serial.printf("[INFO]\tSensor In Navigation mode.\n\r");
        Serial.println();
        Serial.println("\t- Press to check fingerprint identity.");
        Serial.println("\t- Long Press to enroll a fingerprint.");
        Serial.println();
    }
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
    Serial.printf("[ERROR]\t%d.\n\r", error);
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
// on_version()
//
// Call when the sensor sends a version string
//
static void on_version(char *version)
{
    // just print the version string
    Serial.printf("\t\t%s\n\r", version);
}

//----------------------------------------------------------------------------
// on_navigation()
//
// Call when the sensor sends a navigation event
//
static void on_navigation(uint16_t gesture)
{
    Serial.printf("[NAVIGATION]\t");
    switch (gesture)
    {
    case CMD_NAV_EVENT_PRESS:
        // Start the Identity mode ..
        if (numberOfTemplates == 0)
        {
            Serial.printf("PRESS -> {No templates enrolled - cannot check identity}\n\r");
            break;
        }

        Serial.printf("PRESS -> {Identity Check - Place Finger on Sensor}\n\r");
        if (mySensor.currentMode() == STATE_NAVIGATION)
            mySensor.requestAbort(); // get out of name mode

        mySensor.setLED(true);
        // start identity mode
        {
            fpc_id_type_t id = {.type = ID_TYPE_ALL, .id = 0};
            fpc_result_t rc = mySensor.requestIdentify(id, 1);
            if (rc != FPC_RESULT_OK)
                Serial.printf("[ERROR]\tFailed to start identity - error: %d\n\r", rc);
        }

        break;

    case CMD_NAV_EVENT_LONG_PRESS:
        // Request the firmware version from the sensor. The sensor will respond
        // with a version event that will call our on_version() function above.
        Serial.printf("LONG PRESS -> {Enroll a finger}\n\r");
        if (mySensor.currentMode() == STATE_NAVIGATION)
            mySensor.requestAbort(); // get out of name mode

        mySensor.setLED(true);
        // start enroll mode
        {
            fpc_id_type_t id = {.type = ID_TYPE_GENERATE_NEW, .id = 0};
            fpc_result_t rc = mySensor.requestEnroll(id);
            if (rc != FPC_RESULT_OK)
                Serial.printf("[ERROR]\tFailed to start enroll - error: %d\n\r", rc);
        }
        break;
    default:
        Serial.println(".");
        break;
    }
}

static void on_identify(bool is_match, uint16_t id)
{
    Serial.printf("[INFO]\tIdentify Result: %s\n\r", is_match ? "MATCH" : "NO MATCH");
    if (is_match)
        Serial.printf("\t\tMatched Template ID: %d\n\r", id);
}

static void on_enroll(uint8_t feedback, uint8_t samples_remaining)
{

    Serial.printf("[INFO]\t\tEnroll samples remaining: %d, feedback: %s (%d)\n\r", samples_remaining,
                  mySensor.getEnrollFeedBackString(feedback), feedback);
}
//----------------------------------------------------------------------------
// on_list_templates()
//

static void on_list_templates(uint16_t num_templates, uint16_t *template_ids)
{
    numberOfTemplates = num_templates;
    Serial.printf("[INFO]\tNumber of templates on the sensor: %d\n\r", num_templates);
    check_sensor_status();
}

//----------------------------------------------------------------------------
// on_status()
static void on_status(uint16_t event, uint16_t state)
{
    // Serial.printf("[STATUS]\tEvent: 0x%04X, State: 0x%04X\n\r", event, state);

    if (event == EVENT_FINGER_LOST)
    {
        // if we are in identity mode, and finger lost, go back to nav mode
        check_sensor_status();
    }
}
// Define our command callbacks the library will call on events from the sensor
static const sfDevFPC2534Callbacks_t cmd_cb = {.on_error = on_error,
                                               .on_status = on_status,
                                               .on_version = on_version,
                                               .on_enroll = on_enroll,
                                               .on_identify = on_identify,
                                               .on_list_templates = on_list_templates,
                                               .on_navigation = on_navigation,
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
    Serial.println(" SparkFun FPC2534 Fingerprint Example - I2C");
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
    }

    delay(200);
}