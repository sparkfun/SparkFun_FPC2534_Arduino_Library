

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

#define UART_RX 32
#define UART_TX 14

bool startNavigation = true;

// Declare our sensor object
SfeFPC2534I2C mySensor;

/* Command callbacks */
static void on_error(uint16_t error)
{
    // hal_set_led_status(HAL_LED_STATUS_ERROR);
    Serial.printf("[ERROR]\t%d.\n\r", error);
}

static void on_status(uint16_t event, uint16_t state)
{

    // do we need to start navigation mode?
    if (mySensor.currentMode() != STATE_NAVIGATION && mySensor.appIsReady())
    {
        Serial.printf("[SET MODE]\tNAVIGATION\n\r");
        startNavigation = false;
        fpc_result_t rc = mySensor.startNavigationMode(0);
        if (rc != FPC_RESULT_OK)
            Serial.printf("Failed to start navigation mode - error: %d\n\r", rc);
    }
    // Serial.printf("[STATUS]\tEvent: %d, State: 0x%04X\n\r", event, state);
}

static void on_version(char *version)
{
    Serial.printf("[VERSION]\t%s\n\r", version);
}

static void on_navigation(int gesture)
{
    Serial.printf("[NAVIGATION]\t");
    switch (gesture)
    {
    case CMD_NAV_EVENT_NONE:
        Serial.printf("NONE\n\r");
        break;
    case CMD_NAV_EVENT_UP:
        Serial.printf("UP\n\r");
        break;
    case CMD_NAV_EVENT_DOWN:
        Serial.printf("DOWN\n\r");
        break;
    case CMD_NAV_EVENT_RIGHT:
        Serial.printf("RIGHT\n\r");
        break;
    case CMD_NAV_EVENT_LEFT:
        Serial.printf("LEFT\n\r");
        break;
    case CMD_NAV_EVENT_PRESS: {
        Serial.printf("PRESS\n\r");
        break;
    }
    case CMD_NAV_EVENT_LONG_PRESS:
        Serial.printf("LONG PRESS ->{Get Version}\n\r");
        mySensor.requestVersion();
        break;
    default:
        Serial.printf("UNKNOWN\n\r");
        break;
    }
}
static void on_finger_change(bool present)
{
    // Serial.printf("[FINGER]\t%s\n\r", present ? "PRESENT" : "NOT PRESENT");
}
static const sfDevFPC2534Callbacks_t cmd_cb = {.on_error = on_error,
                                               .on_status = on_status,
                                               .on_version = on_version,
                                               .on_navigation = on_navigation,
                                               .on_finger_change = on_finger_change};

void reset_sensor(void)
{
    // Reset the sensor by toggling the reset pin
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, LOW);  // Set reset pin low
    delay(10);                   // Wait for 10 ms
    digitalWrite(RST_PIN, HIGH); // Set reset pin high
    delay(250);                  // Wait for sensor to initialize
}

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

    // Initialize the I2C communication
    Wire.begin();

    // Reset the sensor to ensure it's in a known state - by default this also triggers the
    // sensor to send a status message
    reset_sensor();
    // delay(1000);
    Wire.beginTransmission(kFPC2534DefaultAddress);
    if (Wire.endTransmission() != 0)
    {
        Serial.println("Touch Sensor not found on I2C bus. HALT");
        while (1)
        {
            delay(1000); // Wait indefinitely if device is not found
        }
    }
    else
        Serial.println("Touch Sensor found on I2C bus.");

    // Initialize the sensor
    if (!mySensor.begin(kFPC2534DefaultAddress, Wire, 0, IRQ_PIN))
    {
        Serial.println("FPC2534 not found. Check wiring. HALT.");
        while (1)
            delay(1000);
    }
    Serial.println("FPC2534 initialized.");

    // set the callbacks for the sensor library to call
    mySensor.setCallbacks(cmd_cb);
    reset_sensor();

    Serial.println("Fingerprint system initialized.");
}

void loop()
{

    // Call the library to process the next response from the sensor
    fpc_result_t rc = mySensor.processNextResponse();
    if (rc != FPC_RESULT_OK && rc != FPC_PENDING_OPERATION)
    {
        Serial.printf("[ERROR] Processing Error: %d\n\r", rc);
    }

    delay(200);
}