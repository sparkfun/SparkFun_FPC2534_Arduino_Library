

#include <Arduino.h>
#include <Wire.h>

#include "SparkFun_FPC2534.h"

// esp32 thing plus c
#define RST_PIN 14
#define UART_RX 16
#define UART_TX 17

uint16_t next_op_state = STATE_NAVIGATION;
uint16_t current_state = 0;

// Declare our sensor object
SfeFPC2534UART mySensor;

/* Command callbacks */
static void on_error(uint16_t error)
{
    // hal_set_led_status(HAL_LED_STATUS_ERROR);
    Serial.printf("[ERROR]\t%d.\n\r", error);
}

static void on_status(uint16_t event, uint16_t state)
{

    if (event == EVENT_FINGER_LOST)
    {
        if (current_state == STATE_IDENTIFY)
        {
            // back to nav
            mySensor.requestAbort();
            next_op_state = STATE_NAVIGATION;
        }
    }
    else if (next_op_state != 0 && (state & STATE_APP_FW_READY) == STATE_APP_FW_READY)
    {
        if (next_op_state == STATE_NAVIGATION)
        {
            Serial.printf("Starting navigation\n\r");
            next_op_state = 0; // clear
            fpc_result_t rc = mySensor.startNavigationMode(0);
            if (rc != FPC_RESULT_OK)
                Serial.printf("Failed to start navigation mode - error: %d\n\r", rc);
            else
                current_state = STATE_NAVIGATION;
        }
        else if (next_op_state == STATE_IDENTIFY)
        {
            Serial.printf("Starting identify\n\r");
            fpc_id_type_t id_type = {ID_TYPE_ALL, 0};
            fpc_result_t rc = mySensor.requestIdentify(id_type, 0);
            if (rc != FPC_RESULT_OK)
                Serial.printf("Failed to start identify - error: %d\n\r", rc);
            else
                current_state = STATE_IDENTIFY;
            next_op_state = 0; // clear
        }
    }
    Serial.printf("[STATUS]\tEvent: %d, State: 0x%04X\n\r", event, state);
}

static void on_version(char *version)
{
    Serial.printf("[VERSION]\t%s\n\r", version);
}

static void on_enroll(uint8_t feedback, uint8_t samples_remaining)
{
}

static void on_identify(int is_match, uint16_t id)
{
    if (is_match)
        Serial.printf("Identify match on id %d\n\r", id);
    else
        Serial.printf("Identify no match\n\r");
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
    case CMD_NAV_EVENT_PRESS: {
        // Serial.printf("PRESS\n\r");
        Serial.printf("PRESS ->{Check Identify}\n\r");
        next_op_state = STATE_IDENTIFY;
        // change modes - abort
        mySensor.requestAbort();

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
    delay(150);                  // Wait for sensor to initialize
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

    // setup the serial/uart for the sensor
    Serial1.setRxBufferSize(512);
    Serial1.begin(921600, SERIAL_8N1, UART_RX, UART_TX);
    delay(100);
    Serial.println("Initializing UART");
    for (uint32_t startMS = millis(); !Serial1 && (millis() - startMS < 5000);) // Wait for the serial port to be ready
        delay(200);

    // Initialize the sensor
    if (!mySensor.begin(Serial1))
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