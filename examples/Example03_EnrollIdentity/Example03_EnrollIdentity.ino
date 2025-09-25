

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

// State flags to manage sensor startup/state
// TODO - just copied from the FPC demo - cleanup
#define N_FINGERS_TO_ENROLL 2

/* Application states */
typedef enum
{
    APP_STATE_WAIT_READY = 0,
    APP_STATE_WAIT_VERSION,
    APP_STATE_WAIT_LIST_TEMPLATES,
    APP_STATE_WAIT_ENROLL,
    APP_STATE_WAIT_IDENTIFY,
    APP_STATE_WAIT_ABORT,
    APP_STATE_WAIT_DELETE_TEMPLATES
} app_state_t;

static int quit = 0;
/* Current application state */
static app_state_t app_state = APP_STATE_WAIT_READY;
/* Set after device ready status is received */
static int device_ready = 0;
/* Set after version command response is received */
static int version_read = 0;
/* Set after list templates command response is received */
static int list_templates_done = 0;
/* Updated at each status event from device */
static uint16_t device_state = 0;
/* Set after list templates command response is received */
static int n_templates_on_device = 0;
/* Number of fingers left to enroll */
static int n_fingers_to_enroll = N_FINGERS_TO_ENROLL;
// Declare our sensor object
SfeFPC2534I2C mySensor;

static const char *get_enroll_feedback_str_(uint8_t feedback)
{
    switch (feedback)
    {
    case ENROLL_FEEDBACK_DONE:
        return "Done";
    case ENROLL_FEEDBACK_PROGRESS:
        return "Progress";
    case ENROLL_FEEDBACK_REJECT_LOW_QUALITY:
        return "Reject.LowQuality";
    case ENROLL_FEEDBACK_REJECT_LOW_COVERAGE:
        return "Reject.LowCoverage";
    case ENROLL_FEEDBACK_REJECT_LOW_MOBILITY:
        return "Reject.LowMobility";
    case ENROLL_FEEDBACK_REJECT_OTHER:
        return "Reject.Other";
    case ENROLL_FEEDBACK_PROGRESS_IMMOBILE:
        return "Progress.Immobile";
    default:
        break;
    }
    return "Unknown";
}
/* Command callbacks */
static void on_error(uint16_t error)
{
    // hal_set_led_status(HAL_LED_STATUS_ERROR);
    Serial.printf("[ERROR]\t%d.\n\r", error);
}

static void on_status(uint16_t event, uint16_t state)
{

    if (mySensor.appIsReady())
        device_ready = 1;
    device_state = state;
    // Serial.printf("[STATUS]\tEvent: %d, State: 0x%04X\n\r", event, state);
}

static void on_version(char *version)
{
    Serial.printf("[VERSION]\t%s\n\r", version);
    version_read = 1;
}

static void on_enroll(uint8_t feedback, uint8_t samples_remaining)
{
    Serial.printf("Enroll samples remaining: %d, feedback: %s (%d)\n\r", samples_remaining,
                  get_enroll_feedback_str_(feedback), feedback);
}

static void on_identify(int is_match, uint16_t id)
{
    if (is_match)
    {
        mySensor.setLED(true);
        Serial.printf("MATCH {finger %d}\n\r", id);
    }
    else
    {
        mySensor.setLED(false);
        Serial.printf("NO MATCH\n\r");
    }
}

static void on_list_templates(int num_templates, uint16_t *template_ids)
{
    Serial.printf("Found %d template(s) on device\n\r", num_templates);

    list_templates_done = 1;
    n_templates_on_device = num_templates;
}

static const sfDevFPC2534Callbacks_t cmd_cb = {.on_error = on_error,
                                               .on_status = on_status,
                                               .on_version = on_version,
                                               .on_enroll = on_enroll,
                                               .on_identify = on_identify,
                                               .on_list_templates = on_list_templates};

static void process_state(void)
{
    app_state_t next_state = app_state;

    switch (app_state)
    {
    case APP_STATE_WAIT_READY:
        if (device_ready)
        {
            next_state = APP_STATE_WAIT_VERSION;
            mySensor.requestVersion();
        }
        break;
    case APP_STATE_WAIT_VERSION:
        if (version_read)
        {
            next_state = APP_STATE_WAIT_LIST_TEMPLATES;
            mySensor.requestListTemplates();
        }
        break;
    case APP_STATE_WAIT_LIST_TEMPLATES:
        if (list_templates_done)
        {
            if (n_templates_on_device < N_FINGERS_TO_ENROLL)
            {
                fpc_id_type_t id_type = {ID_TYPE_GENERATE_NEW, 0};
                n_fingers_to_enroll = N_FINGERS_TO_ENROLL - n_templates_on_device;
                Serial.printf("\n\rStarting enroll %d fingers\n\r", n_fingers_to_enroll);
                next_state = APP_STATE_WAIT_ENROLL;
                mySensor.requestEnroll(id_type);
            }
            else
            {
                fpc_id_type_t id_type = {ID_TYPE_ALL, 0};
                Serial.printf("\n\rStarting identify\r\n");
                next_state = APP_STATE_WAIT_IDENTIFY;
                mySensor.requestIdentify(id_type, 0);
            }
        }
        break;
    case APP_STATE_WAIT_ENROLL:
        if ((device_state & STATE_ENROLL) == 0)
        {
            Serial.printf("Enroll one finger done.\r\n");
            n_fingers_to_enroll--;
            if (n_fingers_to_enroll > 0)
            {
                fpc_id_type_t id_type = {ID_TYPE_GENERATE_NEW, 0};
                Serial.printf("Starting enroll\r\n");
                mySensor.requestEnroll(id_type);
            }
            else
            {
                fpc_id_type_t id_type = {ID_TYPE_ALL, 0};
                Serial.printf("starting identify\r\n");
                next_state = APP_STATE_WAIT_IDENTIFY;
                mySensor.requestIdentify(id_type, 0);
            }
        }
        break;
    case APP_STATE_WAIT_IDENTIFY:
        if ((device_state & STATE_IDENTIFY) == 0)
        {
            fpc_id_type_t id_type = {ID_TYPE_ALL, 0};
            delay(100);
            mySensor.requestIdentify(id_type, 0);
        }
        break;
    case APP_STATE_WAIT_ABORT:
        if ((device_state & (STATE_ENROLL | STATE_IDENTIFY)) == 0)
        {
            fpc_id_type_t id_type = {ID_TYPE_ALL, 0};
            Serial.printf("Deleting templates.\r\n");
            next_state = APP_STATE_WAIT_DELETE_TEMPLATES;
            mySensor.requestDeleteTemplate(id_type);
        }
        break;
    // Will run after next status event is received in response to delete template request.
    case APP_STATE_WAIT_DELETE_TEMPLATES: {
        fpc_id_type_t id_type = {ID_TYPE_GENERATE_NEW, 0};
        n_fingers_to_enroll = N_FINGERS_TO_ENROLL;
        Serial.printf("Starting enroll.\r\n");
        next_state = APP_STATE_WAIT_ENROLL;
        mySensor.requestEnroll(id_type);
        break;
    }
    default:
        break;
    }

    if (next_state != app_state)
    {
        Serial.printf("State transition %d -> %d\n\r", app_state, next_state);
        app_state = next_state;
    }
}
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

    // is data available for processing?
    if (mySensor.isDataAvailable())
    {
        // Call the library to process the next response from the sensor
        fpc_result_t rc = mySensor.processNextResponse();
        if (rc != FPC_RESULT_OK && rc != FPC_PENDING_OPERATION)
            Serial.printf("[ERROR] Processing Error: %d\n\r", rc);
        else
            process_state();
    }

    delay(200);
}