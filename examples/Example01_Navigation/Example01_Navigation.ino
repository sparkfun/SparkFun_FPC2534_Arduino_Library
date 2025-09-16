

#include <Arduino.h>
#include <Wire.h>

#include "SparkFun_FPC2534.h"

// esp32 thing plus
#define IRQ_PIN 16
#define RST_PIN 21

#define UART_RX 32
#define UART_TX 14

bool in_navigation_mode = false;

// Declare our sensor object
SfeFPC2534I2C mySensor;

/* Command callbacks */
static void on_error(uint16_t error)
{
    // hal_set_led_status(HAL_LED_STATUS_ERROR);
    Serial.printf("Got error %d.\n\r", error);
}

static void on_status(uint16_t event, uint16_t state)
{
    if (!in_navigation_mode)
    {
        Serial.printf("Starting navigation\n\r");
        in_navigation_mode = true;
        fpc_result_t rc = mySensor.startNavigationMode(0);
        if (rc != FPC_RESULT_OK)
            Serial.printf("Failed to start navigation mode - error: %d\n\r", rc);
    }
    if (event == EVENT_FINGER_LOST)
        Serial.printf("\n\r--------------------------------------------------------------\n\r");
}

static void on_version(char *version)
{
    Serial.printf("Version: %s\n\r", version);
}

static void on_enroll(uint8_t feedback, uint8_t samples_remaining)
{
}

static void on_identify(int is_match, uint16_t id)
{
}

static void on_list_templates(int num_templates, uint16_t *template_ids)
{
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
    case CMD_NAV_EVENT_PRESS:
        Serial.printf("PRESS\n\r");
        break;
    case CMD_NAV_EVENT_LONG_PRESS:
        Serial.printf("LONG PRESS\n\r");
        break;
    default:
        Serial.printf("UNKNOWN\n\r");
        break;
    }
}

static const sfDevFPC2534Callbacks_t cmd_cb = {.on_error = on_error,
                                               .on_status = on_status,
                                               .on_version = on_version,
                                               .on_enroll = on_enroll,
                                               .on_identify = on_identify,
                                               .on_list_templates = on_list_templates,
                                               .on_navigation = on_navigation};

void reset_sensor(void)
{
    // Reset the sensor by toggling the reset pin
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, LOW);  // Set reset pin low
    delay(10);                   // Wait for 10 ms
    digitalWrite(RST_PIN, HIGH); // Set reset pin high
    delay(150);                  // Wait for 100 ms to allow the sensor to reset
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

    // Initialize the sensor
    if (!mySensor.begin(kFPC2534DefaultAddress, Wire, 0, IRQ_PIN))
    {
        Serial.println("FPC2534 not found. Check wiring. HALT.");
        while (1)
            ;
    }
    Serial.println("FPC2534 found.");

    // set the callbacks for the sensor library to call
    mySensor.setCallbacks(cmd_cb);

    // Reset the sensor to ensure it's in a known state - by default this also triggers the
    // sensor to send a status message
    reset_sensor();

    Serial.println("Fingerprint system initialized.");
}

void loop()
{

    // Call the library to process the next response from the sensor
    fpc_result_t rc = mySensor.processNextResponse();
    if (rc != FPC_RESULT_OK && rc != FPC_PENDING_OPERATION)
    {
        Serial.printf("Error processing response: %d\n\r", rc);
    }

    delay(200);
}