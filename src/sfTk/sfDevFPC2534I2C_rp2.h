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

#include "sfDevFPC2534I2C.h"
// ESP32 implementation for the FPC2534 I2C communication class - read protocol.

#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/i2c.h>

class sfDevFPC2534I2C_Helper : public sfDevFPC2534I2C_IRead
{
  public:
    sfDevFPC2534I2C_Helper()
        : _device_address{0}, _i2cBusNumber{0}, _isInitialized{false}, _pendingStop{false}, _timeOutMillis{5000}
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

        bool restart0 = i2c1->restart_on_next;

        i2c1->restart_on_next = false;
        int rc = i2c_read_blocking_until(__WIRE0_DEVICE, _device_address, data, len, false,
                                         make_timeout_time_ms(_timeOutMillis));
        i2c1->restart_on_next = restart0;
        _pendingStop = false;

        if (rc == PICO_ERROR_GENERIC || rc == PICO_ERROR_TIMEOUT)
            len = 0;

        return len;
    }

    // For the FPC data, the first two bytes are the length of the data to follow. So this method reads in
    // in the length and returns it. This method is the "start" of a FPC data read operation. It doesn't
    // stop/end the I2C read operation, that is done in the readPayload() method.
    //--------------------------------------------------------------------------------------------
    uint16_t readTransferSize(uint8_t device_address)
    {
        if (!_isInitialized)
            return 0;

        _device_address = device_address;
        uint16_t theSize = 0;
        int rc = i2c_read_blocking_until(__WIRE0_DEVICE, device_address, (uint8_t *)&theSize, sizeof(theSize), true,
                                         make_timeout_time_ms(_timeOutMillis));

        if (rc == PICO_ERROR_GENERIC || rc == PICO_ERROR_TIMEOUT)
        {

            theSize = 0;
        }
        else
            _pendingStop = true;

        return theSize;
    }

  private:
    uint8_t _device_address;
    uint8_t _i2cBusNumber;
    bool _isInitialized;
    bool _pendingStop;
    uint16_t _timeOutMillis;
};

#endif