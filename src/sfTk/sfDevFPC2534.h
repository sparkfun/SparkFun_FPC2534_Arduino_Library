/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#pragma once

// from the FPC SDK
#include "fpc_api.h"

#include "sfDevFPC2534IComm.h"
#include <Arduino.h>

// Define response types for the responce types
typedef struct
{
    void (*on_error)(uint16_t error);
    void (*on_status)(uint16_t event, uint16_t state);
    void (*on_version)(char *version);
    void (*on_enroll)(uint8_t feedback, uint8_t samples_remaining);
    void (*on_identify)(bool is_match, uint16_t id);
    void (*on_list_templates)(uint16_t num_templates, uint16_t *template_ids);
    void (*on_navigation)(uint16_t gesture);
    void (*on_gpio_control)(uint8_t state);
    void (*on_system_config_get)(fpc_system_config_t *cfg);
    void (*on_bist_done)(uint16_t test_verdict);
    void (*on_data_transfer_done)(uint8_t *data, size_t size);
    void (*on_mode_change)(uint16_t new_mode);
    void (*on_finger_change)(bool present);
    void (*on_is_ready_change)(bool isReady);

} sfDevFPC2534Callbacks_t;

class sfDevFPC2534
{
  public:
    sfDevFPC2534();

    /**
     * @brief Populate and transfer a CMD_STATUS request.
     *
     * @return Result Code
     */
    fpc_result_t requestStatus(void);

    /**
     * @brief Populate and transfer a CMD_VERSION request.
     *
     * @return Result Code
     */
    fpc_result_t requestVersion(void);

    /**
     * @brief Populate and transfer a CMD_ENROLL request.
     *
     * id type can be ID_TYPE_SPECIFIED or ID_TYPE_GENERATE_NEW
     *
     * @param id The User ID to be used for the new template.
     * @return Result Code
     */
    fpc_result_t requestEnroll(fpc_id_type_t &id);

    /**
     * @brief Populate and transfer a CMD_IDENTIFY Request.
     *
     * id type can be ID_TYPE_SPECIFIED or ID_TYPE_ALL
     *
     * @param id The User ID to be used for the new template.
     * @param tag Operation tag. Will be returned in the response.
     *
     * @return Result Code
     */
    fpc_result_t requestIdentify(fpc_id_type_t &id, uint16_t tag);

    /**
     * @brief Populate and transfer a CMD_ABORT request.
     *
     * @return Result Code
     */
    fpc_result_t requestAbort(void);

    /**
     * @brief Populate and transfer a CMD_LIST_TEMPLATES request.
     *
     * @return Result Code
     */
    fpc_result_t requestListTemplates(void);

    /**
     * @brief Populate and transfer a CMD_DEL_TEMPLATE request.
     *
     * id type can be ID_TYPE_SPECIFIED or ID_TYPE_ALL
     *
     * @param id The User ID to be deleted.
     *
     * @return Result Code
     */
    fpc_result_t requestDeleteTemplate(fpc_id_type_t &id);

    /**
     * @brief Populate and transfer a CMD_RESET request.
     *
     * @return Result Code
     */
    fpc_result_t sendReset(void);

    /**
     * @brief Populate and transfer a CMD_SET_CRYPTO_KEY request.
     *
     * @return Result Code
     */
    // fpc_result_t requestCryptoKey(void);

    /**
     * @brief Populate and transfer a CMD_NAVIGATION request.
     *
     * Starts the navigation mode.
     *
     * @param orientation Orientation in 90 degrees per step (0-3).
     *
     * @return Result Code
     */
    fpc_result_t startNavigationMode(uint8_t orientation);

    /**
     * @brief Populate and transfer a CMD_BIST request.
     *
     * Runs the BuiltIn Self Test.
     *
     * @return Result Code
     */
    fpc_result_t startBuiltInSelfTest(void);

    /**
     * @brief Populate and transfer a CMD_GPIO_CONTROL request for SET.
     *
     * Configure gpio pins.
     *
     * @param pin   Pin to configure (see product specification).
     * @param mode  Mode selection (GPIO_CONTROL_MODE_*).
     * @param state State of pin if output (GPIO_CONTROL_STATE_*).
     *
     * @return Result Code
     */
    fpc_result_t requestSetGPIO(uint8_t pin, uint8_t mode, uint8_t state);

    /**
     * @brief Populate and transfer a CMD_GPIO_CONTROL request for GET.
     *
     * Configure gpio pins.
     *
     * @param pin Pin to get state of (see product specification).
     *
     * @return Result Code
     */
    fpc_result_t requestGetGPIO(uint8_t pin);

    /**
     * @brief Populate and transfer a CMD_SET_SYSTEM_CONFIG request.
     *
     * Configure various system settings.
     *
     * @param cfg Pointer to ::fpc_system_config_t.
     *
     * @return Result Code
     */
    fpc_result_t setSystemConfig(fpc_system_config_t *cfg);

    /**
     * @brief Populate and transfer a CMD_GET_SYSTEM_CONFIG request.
     *
     * Configure various system settings.
     *
     * @param type One of FPC_SYS_CFG_TYPE_*.
     *
     * @return Result Code
     */
    fpc_result_t requestGetSystemConfig(uint8_t type);

    // /**
    //  * @brief Populate and transfer a CMD_PUT_TEMPLATE_DATA request
    //  *
    //  * Send template to device.
    //  *
    //  * @param id   Template id.
    //  * @param data Pointer to template data.
    //  * @param size Size of template data.
    //  *
    //  * @return Result Code
    //  */
    // fpc_result_t requestPutTemplateData(uint16_t id, uint8_t *data, size_t size);

    // /**
    //  * @brief Populate and transfer a CMD_GET_TEMPLATE_DATA request.
    //  *
    //  * Read template from device. This function will allocate memory for the template and
    //  * return data and size in the callback function (on_data_transfer_done).
    //  *
    //  * @param id Template id.
    //  *
    //  * @return Result Code
    //  */
    // fpc_result_t requestGetTemplateData(uint16_t id);

    /**
     * @brief Populate and transfer a CMD_FACTORY_RESET request.
     *
     * Perform factory reset (system config flags must be set to support this).
     *
     * @return Result Code
     */
    fpc_result_t factoryReset(void);

    uint16_t currentMode(void) const
    {
        return _current_state & (STATE_ENROLL | STATE_IDENTIFY | STATE_NAVIGATION);
    }
    bool isFingerPresent(void) const
    {
        return _finger_present;
    }

    // for the library to actually work, user provided callbacks are needed ...
    void setCallbacks(const sfDevFPC2534Callbacks_t &callbacks)
    {
        _callbacks = callbacks;
    }

    // initialize the library
    bool initialize(sfDevFPC2534IComm &comm)
    {
        _comm = &comm;
        return true;
    }
    bool isReady(void) const
    {
        return (_current_state & STATE_APP_FW_READY) == STATE_APP_FW_READY;
    }

    bool isDataAvailable(void) const
    {
        if (_comm == nullptr)
            return false;
        return _comm->dataAvailable();
    }

    void clearData(void)
    {
        if (_comm != nullptr)
            _comm->clearData();
    }
    fpc_result_t setLED(bool on = true);

    fpc_result_t processNextResponse(bool flushNone);
    fpc_result_t processNextResponse(void)
    {
        return processNextResponse(false);
    };

    static const char *getEnrollFeedBackString(uint8_t feedback)
    {
        switch (feedback)
        {
        case ENROLL_FEEDBACK_DONE:
            return "Done";
        case ENROLL_FEEDBACK_PROGRESS:
            return "Progress";
        case ENROLL_FEEDBACK_REJECT_LOW_QUALITY:
            return "Reject - LowQuality";
        case ENROLL_FEEDBACK_REJECT_LOW_COVERAGE:
            return "Reject - LowCoverage";
        case ENROLL_FEEDBACK_REJECT_LOW_MOBILITY:
            return "Reject - LowMobility";
        case ENROLL_FEEDBACK_REJECT_OTHER:
            return "Reject - Other";
        case ENROLL_FEEDBACK_PROGRESS_IMMOBILE:
            return "Progress - Immobile";
        default:
            break;
        }
        return "Unknown";
    }

  private:
    fpc_result_t sendCommand(fpc_cmd_hdr_t &cmd, size_t size);
    fpc_result_t parseStatusCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseVersionCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseEnrollStatusCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseIdentifyCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseListTemplatesCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseNavigationEventCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseGPIOControlCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseGetSystemConfigCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseBISTCommand(fpc_cmd_hdr_t *, size_t);
    fpc_result_t parseCommand(uint8_t *frame_payload, size_t payload_size);

    bool checkForNoneEvent(uint8_t *payload, size_t size);
    fpc_result_t flushNoneEvent(void);

    sfDevFPC2534IComm *_comm = nullptr;
    sfDevFPC2534Callbacks_t _callbacks;

    // current state of the sensor
    uint16_t _current_state = 0;

    // Is a finger present?
    bool _finger_present = false;
};