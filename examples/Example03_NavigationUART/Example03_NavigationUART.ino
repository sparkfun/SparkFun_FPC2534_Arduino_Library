

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
// #define IRQ_PIN 11
#define RST_PIN 12

// #define UART_RX 32
// #define UART_TX 14

// State flags to manage sensor startup/state
bool startNavigation = true;

// Used to track LED state
bool ledState = false;

// Declare our sensor object
SfeFPC2534UART mySensor;

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
    Serial.print("[ERROR]\t");
    Serial.print(error);
    Serial.print(" - 0x");
    Serial.println(error, HEX);
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
                Serial.print("[ERROR]\tFailed to start navigation mode - error: ");
                Serial.println(rc);
                return;
            }

            Serial.println("[SETUP]\tSensor In Navigation mode");
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
        Serial.println("UP ^");
        break;
    case CMD_NAV_EVENT_DOWN:
        Serial.println("DOWN v");
        break;
    case CMD_NAV_EVENT_RIGHT:
        Serial.println("RIGHT >");
        break;
    case CMD_NAV_EVENT_LEFT:
        Serial.println("LEFT <");
        break;
    case CMD_NAV_EVENT_PRESS:
        // Toggle the on-board  LED
        Serial.print("PRESS -> {LED");
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

// Define our command callback structure - initialize to 0/null.
// assigin our callback methods in setup
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
    Serial.println(" SparkFun FPC2534 Navigation Example - UART");
    Serial.println("----------------------------------------------------------------");
    Serial.println();

    // Setup our callback functions structure
    cmd_cb.on_error = on_error;
    cmd_cb.on_version = on_version;
    cmd_cb.on_navigation = on_navigation;
    cmd_cb.on_is_ready_change = on_is_ready_change;

    // Initialize the UART/Serial communication

    // The internal UART buffer can fill up quickly and overflow. As such, increase its size.
    // This example is supporting ESP32 and RP2040 based boards - adjust as needed for other platforms.
#if defined(ARDUINO_ARCH_RP2040)
    Serial1.setFIFOSize(512);
#elif defined(ESP32)
    Serial1.setRxBufferSize(512);
#endif

    // Setup Serial1 for communication with the FPC2534
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
        Serial.println("[ERROR]\tFailure to start the sensor - HALT");
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
        Serial.print("[ERROR] Processing Error: ");
        Serial.println(rc);
    }

    delay(200);
}