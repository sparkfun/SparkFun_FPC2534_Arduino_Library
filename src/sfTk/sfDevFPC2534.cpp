/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include "sfDevFPC2534.h"

sfDevFPC2534::sfDevFPC2534() : _comm{nullptr}, _callbacks{0}, _current_state{0}, _finger_present{false}
{
}
//--------------------------------------------------------------------------------------------
// Internal send command method...
fpc_result_t sfDevFPC2534::sendCommand(fpc_cmd_hdr_t &cmd, size_t size)
{
    if (_comm == nullptr)
        return FPC_RESULT_WRONG_STATE;

    // fill in a header
    fpc_frame_hdr_t frameHeader = {0};
    frameHeader.version = FPC_FRAME_PROTOCOL_VERSION;
    frameHeader.type = FPC_FRAME_TYPE_CMD_REQUEST;
    frameHeader.flags = FPC_FRAME_FLAG_SENDER_HOST;
    frameHeader.payload_size = (uint16_t)size;

    fpc_result_t rc = _comm->write((uint8_t *)&frameHeader, sizeof(fpc_frame_hdr_t));

    if (rc == FPC_RESULT_OK)
        rc = _comm->write((uint8_t *)&cmd, size);

    return rc;
}
//--------------------------------------------------------------------------------------------
//  Command Requests
//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestStatus(void)
{
    /* Status Command Request has no payload */
    fpc_cmd_hdr_t cmd = {.cmd_id = CMD_STATUS, .type = FPC_FRAME_TYPE_CMD_REQUEST};

    return sendCommand(cmd, sizeof(fpc_cmd_hdr_t));
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestVersion(void)
{
    /* Version Command Request has no payload */
    fpc_cmd_hdr_t cmd = {.cmd_id = CMD_VERSION, .type = FPC_FRAME_TYPE_CMD_REQUEST};

    return sendCommand(cmd, sizeof(fpc_cmd_hdr_t));
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestEnroll(fpc_id_type_t &id)
{
    if (id.type != ID_TYPE_SPECIFIED && id.type != ID_TYPE_GENERATE_NEW)
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_enroll_request_t cmd = {.cmd = {.cmd_id = CMD_ENROLL, .type = FPC_FRAME_TYPE_CMD_REQUEST}, .tpl_id = id};

    return sendCommand((fpc_cmd_hdr_t &)cmd, sizeof(fpc_cmd_enroll_request_t));
}
//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestIdentify(fpc_id_type_t &id, uint16_t tag)
{
    if (id.type != ID_TYPE_SPECIFIED && id.type != ID_TYPE_ALL)
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_identify_request_t cmd = {
        .cmd = {.cmd_id = CMD_IDENTIFY, .type = FPC_FRAME_TYPE_CMD_REQUEST}, .tpl_id = id, .tag = tag};

    return sendCommand((fpc_cmd_hdr_t &)cmd, sizeof(fpc_cmd_identify_request_t));
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestAbort(void)
{
    /* Abort Command Request has no payload */
    fpc_cmd_hdr_t cmd = {.cmd_id = CMD_ABORT, .type = FPC_FRAME_TYPE_CMD_REQUEST};

    return sendCommand(cmd, sizeof(fpc_cmd_hdr_t));
}
//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestListTemplates(void)
{
    /* List Templates Command Request has no payload */
    fpc_cmd_hdr_t cmd = {.cmd_id = CMD_LIST_TEMPLATES, .type = FPC_FRAME_TYPE_CMD_REQUEST};

    return sendCommand(cmd, sizeof(fpc_cmd_hdr_t));
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestDeleteTemplate(fpc_id_type_t &id)
{
    if (id.type != ID_TYPE_SPECIFIED && id.type != ID_TYPE_ALL)
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_enroll_request_t cmd = {.cmd = {.cmd_id = CMD_DELETE_TEMPLATE, .type = FPC_FRAME_TYPE_CMD_REQUEST},
                                    .tpl_id = id};

    return sendCommand((fpc_cmd_hdr_t &)cmd, sizeof(fpc_cmd_enroll_request_t));
}
//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::sendReset(void)
{
    /* Reset Command Request has no payload */
    fpc_cmd_hdr_t cmd = {.cmd_id = CMD_RESET, .type = FPC_FRAME_TYPE_CMD_REQUEST};

    return sendCommand(cmd, sizeof(fpc_cmd_hdr_t));
}
//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::startNavigationMode(uint8_t orientation)
{
    if (orientation > 3)
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_navigation_request_t cmd = {.cmd = {.cmd_id = CMD_NAVIGATION, .type = FPC_FRAME_TYPE_CMD_REQUEST},
                                        .config = orientation};

    return sendCommand((fpc_cmd_hdr_t &)cmd, sizeof(fpc_cmd_navigation_request_t));
}
//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::startBuiltInSelfTest(void)
{
    /* BIST Command Request has no payload */
    fpc_cmd_hdr_t cmd = {.cmd_id = CMD_BIST, .type = FPC_FRAME_TYPE_CMD_REQUEST};

    return sendCommand(cmd, sizeof(fpc_cmd_hdr_t));
}
//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestSetGPIO(uint8_t pin, uint8_t mode, uint8_t state)
{
    if (mode > GPIO_CONTROL_MODE_INPUT_PULL_DOWN || state > GPIO_CONTROL_STATE_SET)
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_pinctrl_gpio_request_t cmd = {.cmd = {.cmd_id = CMD_GPIO_CONTROL, .type = FPC_FRAME_TYPE_CMD_REQUEST},
                                          .sub_cmd = GPIO_CONTROL_SUB_CMD_SET,
                                          .pin = pin,
                                          .mode = mode,
                                          .state = state};

    return sendCommand((fpc_cmd_hdr_t &)cmd, sizeof(fpc_cmd_pinctrl_gpio_request_t));
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestGetGPIO(uint8_t pin)
{
    fpc_cmd_pinctrl_gpio_request_t cmd = {.cmd = {.cmd_id = CMD_GPIO_CONTROL, .type = FPC_FRAME_TYPE_CMD_REQUEST},
                                          .sub_cmd = GPIO_CONTROL_SUB_CMD_GET,
                                          .pin = pin,
                                          .mode = 0,
                                          .state = 0};

    return sendCommand((fpc_cmd_hdr_t &)cmd, sizeof(fpc_cmd_pinctrl_gpio_request_t));
}
//--------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::setSystemConfig(fpc_system_config_t *cfg)
{
    if (cfg == nullptr)
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_set_config_request_t cmd = {.cmd = {.cmd_id = CMD_SET_SYSTEM_CONFIG, .type = FPC_FRAME_TYPE_CMD_REQUEST},
                                        .cfg = *cfg};

    return sendCommand((fpc_cmd_hdr_t &)cmd, sizeof(fpc_cmd_set_config_request_t));
}
//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::requestGetSystemConfig(uint8_t type)
{
    if (type > FPC_SYS_CFG_TYPE_CUSTOM)
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_get_config_request_t cmd = {.cmd = {.cmd_id = CMD_GET_SYSTEM_CONFIG, .type = FPC_FRAME_TYPE_CMD_REQUEST},
                                        .config_type = type};
    return sendCommand((fpc_cmd_hdr_t &)cmd, sizeof(fpc_cmd_get_config_request_t));
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::factoryReset(void)
{
    /* Factory Reset Command Request has no payload */
    fpc_cmd_hdr_t cmd = {.cmd_id = CMD_FACTORY_RESET, .type = FPC_FRAME_TYPE_CMD_REQUEST};
    return sendCommand(cmd, sizeof(fpc_cmd_hdr_t));
}

//--------------------------------------------------------------------------------------------
//  Internal parse command methods
//--------------------------------------------------------------------------------------------

/* Command Responses / Events */

fpc_result_t sfDevFPC2534::parseStatusCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{

    if (size != sizeof(fpc_cmd_status_response_t))
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_status_response_t *status = (fpc_cmd_status_response_t *)cmd_hdr;
    // if (status->state & STATE_SECURE_INTERFACE)
    // {
    //     /* The device supports secure interface. Set the flag to indicate
    //        that the secure interface shall be used. */
    //     use_secure_interface = true;
    // }
    // else
    // {
    // use_secure_interface = false;
    // }
    // Serial.printf("[STATUS]\tEvent: 0x%04X, State: 0x%04X, AppFail: 0x%04X\n\r", status->event, status->state,
    //               status->app_fail_code);
    // if we have an error code, just call the error callback and exit
    if (status->app_fail_code != 0)
    {
        if (_callbacks.on_error)
            _callbacks.on_error(status->app_fail_code);
        return FPC_RESULT_OK;
    }

    uint16_t prev_state = _current_state;
    // stash our new state
    _current_state = status->state;

    // mode change
    if (currentMode() != (prev_state & (STATE_ENROLL | STATE_IDENTIFY | STATE_NAVIGATION)) && _callbacks.on_mode_change)
        _callbacks.on_mode_change(currentMode());

    // finger present change
    if (isFingerPresent() != ((prev_state & STATE_FINGER_DOWN) == STATE_FINGER_DOWN))
    {
        if (_callbacks.on_finger_change)
            _callbacks.on_finger_change(isFingerPresent());
    }

    // Is there an error code?
    if (_callbacks.on_status)
        _callbacks.on_status(status->event, status->state);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseVersionCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{
    fpc_cmd_version_response_t *ver = (fpc_cmd_version_response_t *)cmd_hdr;

    // The full size of the command must include the length of the version string (unset array)

    if (size != sizeof(fpc_cmd_version_response_t) + ver->version_str_len)
        return FPC_RESULT_INVALID_PARAM;

    if (_callbacks.on_version)
        _callbacks.on_version(ver->version_str);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseEnrollStatusCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{
    if (size != sizeof(fpc_cmd_enroll_status_response_t))
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_enroll_status_response_t *status = (fpc_cmd_enroll_status_response_t *)cmd_hdr;

    if (_callbacks.on_enroll)
        _callbacks.on_enroll(status->feedback, status->samples_remaining);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseIdentifyCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{
    if (size != sizeof(fpc_cmd_identify_status_response_t))
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_identify_status_response_t *id_res = (fpc_cmd_identify_status_response_t *)cmd_hdr;

    if (_callbacks.on_identify)
        _callbacks.on_identify(id_res->match == IDENTIFY_RESULT_MATCH, id_res->tpl_id.id);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseListTemplatesCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{
    fpc_cmd_template_info_response_t *list = (fpc_cmd_template_info_response_t *)cmd_hdr;

    if (size != sizeof(fpc_cmd_template_info_response_t) + (sizeof(uint16_t) * list->number_of_templates))
        return FPC_RESULT_INVALID_PARAM;

    if (_callbacks.on_list_templates)
        _callbacks.on_list_templates(list->number_of_templates, list->template_id_list);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseNavigationEventCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{
    if (size != sizeof(fpc_cmd_navigation_status_event_t))
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_navigation_status_event_t *cmd_nav = (fpc_cmd_navigation_status_event_t *)cmd_hdr;

    if (_callbacks.on_navigation)
        _callbacks.on_navigation(cmd_nav->gesture);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseGPIOControlCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{
    if (size != sizeof(fpc_cmd_pinctrl_gpio_response_t))
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_pinctrl_gpio_response_t *cmd_rsp = (fpc_cmd_pinctrl_gpio_response_t *)cmd_hdr;

    if (_callbacks.on_gpio_control)
        _callbacks.on_gpio_control(cmd_rsp->state);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseGetSystemConfigCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{
    if (size < sizeof(fpc_cmd_get_config_response_t))
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_get_config_response_t *cmd_cfg = (fpc_cmd_get_config_response_t *)cmd_hdr;

    if (_callbacks.on_system_config_get)
        _callbacks.on_system_config_get(&cmd_cfg->cfg);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseBISTCommand(fpc_cmd_hdr_t *cmd_hdr, size_t size)
{
    if (size < sizeof(fpc_cmd_bist_response_t))
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_bist_response_t *cmd_rsp = (fpc_cmd_bist_response_t *)cmd_hdr;

    if (_callbacks.on_bist_done)
        _callbacks.on_bist_done(cmd_rsp->test_verdict);

    return FPC_RESULT_OK;
}

// static fpc_result_t parse_cmd_get_template_data(fpc_cmd_hdr_t *cmd, uint16_t size)
// {
//     fpc_result_t result = FPC_RESULT_OK;
//     fpc_cmd_template_data_response_t *data = (fpc_cmd_template_data_response_t *)cmd;

//     if (size < sizeof(fpc_cmd_template_data_response_t))
//     {
//         fpc_sample_logf("CMD_GET_TEMPLATE_DATA invalid size (%d vs %d)", size,
//                         sizeof(fpc_cmd_template_data_response_t));
//         result = FPC_RESULT_INVALID_PARAM;
//     }

//     if (result == FPC_RESULT_OK)
//     {
//         fpc_sample_logf("Setup Template Data Transfer. Max chunk size = %d", data->max_chunk_size);
//         // Start transfer loop
//         result = fpc_cmd_data_get_setup(data->total_size, data->max_chunk_size);
//     }

//     if (result == FPC_RESULT_OK)
//     {
//         result = fpc_cmd_data_get_request();
//     }

//     return FPC_RESULT_OK;
// }

// static fpc_result_t parse_cmd_put_template_data(fpc_cmd_hdr_t *cmd, uint16_t size)
// {
//     fpc_result_t result = FPC_RESULT_OK;
//     fpc_cmd_template_data_response_t *data = (fpc_cmd_template_data_response_t *)cmd;

//     if (size < sizeof(fpc_cmd_template_data_response_t))
//     {
//         fpc_sample_logf("CMD_PUT_TEMPLATE_DATA invalid size (%d vs %d)", size,
//                         sizeof(fpc_cmd_template_data_response_t));
//         result = FPC_RESULT_INVALID_PARAM;
//     }

//     if (result == FPC_RESULT_OK)
//     {
//         // Start transfer loop
//         result = fpc_cmd_data_put_request(data->max_chunk_size);
//     }

//     return result;
// }

// static fpc_result_t parse_cmd_data_get(fpc_cmd_hdr_t *cmd, uint16_t size)
// {
//     fpc_result_t result = FPC_RESULT_OK;
//     fpc_cmd_data_get_response_t *cmd_rsp = (fpc_cmd_data_get_response_t *)cmd;

//     if (transfer_session.data_buf == NULL)
//     {
//         fpc_sample_logf("CMD_DATA_GET no data buffer allocated");
//         result = FPC_RESULT_INVALID_PARAM;
//     }

//     if (result == FPC_RESULT_OK)
//     {
//         if (size < sizeof(fpc_cmd_data_get_response_t))
//         {
//             fpc_sample_logf("CMD_DATA_GET invalid size (%d vs %d)", size, sizeof(fpc_cmd_data_get_response_t));
//             result = FPC_RESULT_INVALID_PARAM;
//         }
//     }

//     if (result == FPC_RESULT_OK)
//     {
//         if (size != (sizeof(fpc_cmd_data_get_response_t) + cmd_rsp->data_size))
//         {
//             fpc_sample_logf("CMD_DATA_GET invalid size, incl data (%d vs %d)", size,
//                             sizeof(fpc_cmd_data_get_response_t) + cmd_rsp->data_size);
//             result = FPC_RESULT_INVALID_PARAM;
//         }
//     }

//     if (result == FPC_RESULT_OK)
//     {
//         fpc_sample_logf("Data: Got %d, remaining %d", cmd_rsp->data_size, cmd_rsp->remaining_size);

//         memcpy(transfer_session.data_buf + transfer_session.transferred_size, cmd_rsp->data, cmd_rsp->data_size);

//         transfer_session.transferred_size += cmd_rsp->data_size;
//         transfer_session.remaining_size = cmd_rsp->remaining_size;

//         if (cmd_rsp->remaining_size > 0)
//         {
//             fpc_cmd_data_get_request();
//         }
//         else
//         {
//             fpc_sample_logf("CMD_DATA_GET done");
//             if (_callbacks.on_data_transfer_done)
//             {
//                 _callbacks.on_data_transfer_done(transfer_session.data_buf, transfer_session.total_size);
//             }
//             free(transfer_session.data_buf);
//             transfer_session.data_buf = NULL;
//         }
//     }

//     return result;
// }

// static fpc_result_t parse_cmd_data_put(fpc_cmd_hdr_t *cmd, uint16_t size)
// {
//     fpc_result_t result = FPC_RESULT_OK;
//     fpc_cmd_data_put_response_t *cmd_rsp = (fpc_cmd_data_put_response_t *)cmd;

//     if (size < sizeof(fpc_cmd_data_put_response_t))
//     {
//         fpc_sample_logf("CMD_DATA_PUT invalid size (%d vs %d)", size, sizeof(fpc_cmd_data_put_response_t));
//         result = FPC_RESULT_INVALID_PARAM;
//     }

//     if (result == FPC_RESULT_OK)
//     {
//         fpc_sample_logf("DATA Put Response (total put = %d)", cmd_rsp->total_received);

//         if (transfer_session.remaining_size == 0)
//         {
//             fpc_sample_logf("CMD_DATA_PUT done");
//             if (_callbacks.on_data_transfer_done)
//             {
//                 _callbacks.on_data_transfer_done(NULL, 0);
//             }
//         }
//         else
//         {
//             result = fpc_cmd_data_put_request(transfer_session.max_chunk_size);
//         }
//     }

//     return result;
// }

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::parseCommand(uint8_t *payload, size_t size)
{
    if (payload == nullptr || size == 0)
        return FPC_RESULT_INVALID_PARAM;

    fpc_cmd_hdr_t *cmdHeader = (fpc_cmd_hdr_t *)payload;

    // look legit?
    if (cmdHeader->type != FPC_FRAME_TYPE_CMD_EVENT && cmdHeader->type != FPC_FRAME_TYPE_CMD_RESPONSE)
        return FPC_RESULT_INVALID_PARAM;

    switch (cmdHeader->cmd_id)
    {
    case CMD_STATUS:
        return parseStatusCommand(cmdHeader, size);
        break;
    case CMD_VERSION:
        return parseVersionCommand(cmdHeader, size);
        break;
    case CMD_ENROLL:
        return parseEnrollStatusCommand(cmdHeader, size);
        break;
    case CMD_IDENTIFY:
        return parseIdentifyCommand(cmdHeader, size);
        break;
    case CMD_LIST_TEMPLATES:
        return parseListTemplatesCommand(cmdHeader, size);
        break;
    case CMD_NAVIGATION:
        return parseNavigationEventCommand(cmdHeader, size);
        break;
    case CMD_GPIO_CONTROL:
        return parseGPIOControlCommand(cmdHeader, size);
        break;
    case CMD_GET_SYSTEM_CONFIG:
        return parseGetSystemConfigCommand(cmdHeader, size);
        break;
    case CMD_BIST:
        return parseBISTCommand(cmdHeader, size);
        break;
    // case CMD_GET_TEMPLATE_DATA:
    //     return parse_cmd_get_template_data(cmdHeader, size);
    //     break;
    // case CMD_PUT_TEMPLATE_DATA:
    //     return parse_cmd_put_template_data(cmdHeader, size);
    //     break;
    // case CMD_DATA_GET:
    //     return parse_cmd_data_get(cmdHeader, size);
    //     break;
    // case CMD_DATA_PUT:
    //     return parse_cmd_data_put(cmdHeader, size);
    //     break;
    default:
        return FPC_RESULT_INVALID_PARAM;
        break;
    }

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
fpc_result_t sfDevFPC2534::processNextResponse()
{
    if (_comm == nullptr)
        return FPC_RESULT_WRONG_STATE;

    // Check if data is available - no data, just continue
    if (!_comm->dataAvailable())
        return FPC_RESULT_OK;

    fpc_frame_hdr_t frameHeader;

    /* Step 1: Read Frame Header */
    fpc_result_t rc = _comm->read((uint8_t *)&frameHeader, sizeof(fpc_frame_hdr_t));

    // No data? No problem
    if (rc == FPC_RESULT_IO_NO_DATA)
    {
        // response.type = SFE_FPC_RESP_NONE;
        return FPC_RESULT_OK; // No data to process, just return
    }
    else if (rc != FPC_RESULT_OK)
        return rc;

    // Serial.printf("Frame Header: ver 0x%04X, type 0x%02X, flags 0x%04X, payload size %d\n\r",
    // frameHeader.version,
    //               frameHeader.type, frameHeader.flags, frameHeader.payload_size);

    // Sanity check of the header...
    if (frameHeader.version != FPC_FRAME_PROTOCOL_VERSION ||
        ((frameHeader.flags & FPC_FRAME_FLAG_SENDER_FW_APP) == 0) ||
        (frameHeader.type != FPC_FRAME_TYPE_CMD_RESPONSE && frameHeader.type != FPC_FRAME_TYPE_CMD_EVENT))
        return FPC_RESULT_IO_BAD_DATA;

    // okay, lets read the payload
    uint8_t framePayload[frameHeader.payload_size];

    rc = _comm->read(framePayload, frameHeader.payload_size);

    if (rc != FPC_RESULT_OK)
        return rc;

    return parseCommand(framePayload, frameHeader.payload_size);
}

fpc_result_t sfDevFPC2534::setLED(bool ledOn)
{
    if (_comm == nullptr)
        return FPC_RESULT_WRONG_STATE;

    return requestSetGPIO(1, GPIO_CONTROL_MODE_OUTPUT_PP, ledOn ? GPIO_CONTROL_STATE_SET : GPIO_CONTROL_STATE_RESET);
}