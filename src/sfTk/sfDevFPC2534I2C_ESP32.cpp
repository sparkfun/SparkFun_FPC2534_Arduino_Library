/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include "sfDevFPC2534I2C_ESP32.h"

#ifdef ESP32
#include "driver/i2c.h"

uint16_t sfDevFPC2534I2C_ESP32::readPayload(size_t len, uint8_t *inbuffer)
{

    if (!_isInitialized)
        return 0;

    esp_err_t err = ESP_OK;
    uint8_t buffer[256] = {0};

    uint16_t theSize = 0;

    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    if (handle == NULL)
        return 0;

    err = i2c_master_read(handle, (uint8_t *)inbuffer, len, I2C_MASTER_LAST_NACK);
    if (err != ESP_OK)
    {
        goto end;
    }

    i2c_master_stop(handle);
    err = i2c_master_cmd_begin((i2c_port_t)_i2cBusNumber, handle, _timeOutMillis / portTICK_PERIOD_MS);

    if (err == ESP_OK)
        theSize = len;

    _pendingStop = false;

    i2c_cmd_link_delete_static(handle);

end:
    return theSize;
}

// For the FPC data, the first two bytes are the length of the data to follow. So this method reads in
// in the length and returns it. This method is the "start" of a FPC data read operation. It doesn't
// stop/end the I2C read operatoinn, that is done in the readPayload() method.
//

uint16_t sfDevFPC2534I2C_ESP32::readTransferSize(uint8_t device_address)
{
    if (!_isInitialized)
        return 0;

    esp_err_t err = ESP_OK;
    uint8_t buffer[256] = {0};

    uint16_t theSize = 0;

    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    if (handle == NULL)
        return 0;

    // do we need to stop a previous read operation?
    if (_pendingStop)
    {
        i2c_master_stop(handle);
        err = i2c_master_cmd_begin((i2c_port_t)_i2cBusNumber, handle, _timeOutMillis / portTICK_PERIOD_MS);
        i2c_cmd_link_delete_static(handle);
        handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
        if (handle == NULL)
            return 0;
    }

    err = i2c_master_start(handle);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_write_byte(handle, device_address << 1 | I2C_MASTER_READ, true);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_read(handle, (uint8_t *)&theSize, sizeof(theSize), I2C_MASTER_ACK);
    if (err != ESP_OK)
    {
        goto end;
    }

    err = i2c_master_cmd_begin((i2c_port_t)num, handle, _timeOutMillis / portTICK_PERIOD_MS);

    if (err != ESP_OK)
    {
        theSize = 0;
    }
    else
        _pendingStop = true;

end:
    i2c_cmd_link_delete_static(handle);

    return theSize;
}

#endif