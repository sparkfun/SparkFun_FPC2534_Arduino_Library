/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

// I2C helper for the ESP32 platform

#pragma once

#include "sfDevFPC2534I2C.h"
// ESP32 implementation for the FPC2534 I2C communication class - read protocol.

#ifdef ESP32
#include "driver/i2c.h"

class sfDevFPC2534I2C_Helper : public sfDevFPC2534I2C_IRead
{
  public:
    sfDevFPC2534I2C_Helper() : _i2cBusNumber{0}, _isInitialized{false}, _pendingStop{false}, _timeOutMillis{50}
    {
    }
    void initialize(uint8_t i2cBusNumber)
    {
        _i2cBusNumber = i2cBusNumber;
        _isInitialized = true;
        _pendingStop = false;
    }
    //--------------------------------------------------------------------------------------------
    // Read the payload data from the device - this is called after readTransferSize() to get
    // the actual data.
    //--------------------------------------------------------------------------------------------
    uint16_t readPayload(size_t len, uint8_t *data)
    {
        if (!_isInitialized)
            return 0;

        // Create a read command and execute it.  This is a continued read from the previous
        // readTransferSize() call, so we don't need to send a start condition or the device address.
        esp_err_t err = ESP_OK;
        uint8_t buffer[256] = {0};

        uint16_t theSize = 0;

        i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
        if (handle == NULL)
            return 0;

        err = i2c_master_read(handle, (uint8_t *)data, len, I2C_MASTER_LAST_NACK);
        if (err == ESP_OK)
        {

            i2c_master_stop(handle);
            err = i2c_master_cmd_begin((i2c_port_t)_i2cBusNumber, handle, _timeOutMillis / portTICK_PERIOD_MS);

            if (err == ESP_OK)
                theSize = len;

            _pendingStop = false;

            i2c_cmd_link_delete_static(handle);
        }

        return theSize;
    }

    //--------------------------------------------------------------------------------------------
    // For the FPC data, the first two bytes are the length of the data to follow. So this method reads in
    // in the length and returns it. This method is the "start" of a FPC data read operation. It doesn't
    // stop/end the I2C read operation, that is done in the readPayload() method.
    //
    uint16_t readTransferSize(uint8_t device_address)
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

        // build up our read command
        err = i2c_master_start(handle);
        if (err == ESP_OK)
        {
            // address
            err = i2c_master_write_byte(handle, device_address << 1 | I2C_MASTER_READ, true);
            if (err == ESP_OK)
            {
                // read the size
                err = i2c_master_read(handle, (uint8_t *)&theSize, sizeof(theSize), I2C_MASTER_ACK);
                if (err == ESP_OK)
                {
                    // run the command
                    err = i2c_master_cmd_begin((i2c_port_t)_i2cBusNumber, handle, _timeOutMillis / portTICK_PERIOD_MS);

                    if (err != ESP_OK)
                        theSize = 0;
                    else
                        _pendingStop = true;
                }
            }
        }
        i2c_cmd_link_delete_static(handle);

        return theSize;
    }

  private:
    uint8_t _i2cBusNumber;
    bool _isInitialized;
    bool _pendingStop;
    uint16_t _timeOutMillis;
};

#endif