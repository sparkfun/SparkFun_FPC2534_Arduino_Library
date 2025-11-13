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
// RP2 implementation for the FPC2534 I2C communication class - read protocol.

#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/i2c.h>

class sfDevFPC2534I2C_Helper : public sfDevFPC2534I2C_IRead
{
  public:
    sfDevFPC2534I2C_Helper()
        : _device_address{0}, _i2cPort{nullptr}, _isInitialized{false}, _pendingStop{false}, _timeOutMillis{5000}
    {
    }
    void initialize(uint8_t i2cBusNumber)
    {
        // Need to map the provided bus number to the actual i2c port

        if (i2cBusNumber == 0)
        {
            // is a port defined?
#if defined(__WIRE0_DEVICE)
            _i2cPort = __WIRE0_DEVICE;
#else
            _i2cPort = i2c0;
#endif
        }
        else if (i2cBusNumber == 1)
        {
            // is a port defined?
#if defined(__WIRE1_DEVICE)
            _i2cPort = __WIRE1_DEVICE;
#else
            _i2cPort = i2c1;
#endif
        }
        else
        {
            // invalid bus number - for now default to i2c0
            _i2cPort = i2c0;
        }
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

        // Since we want to continue the read operation and not restart, set the restart_on_next flag to false.
        bool restart0 = _i2cPort->restart_on_next;

        _i2cPort->restart_on_next = false;
        int rc =
            i2c_read_blocking_until(_i2cPort, _device_address, data, len, false, make_timeout_time_ms(_timeOutMillis));

        // restore the restart flag to its previous state
        _i2cPort->restart_on_next = restart0;
        _pendingStop = false;

        // Problem?
        if (rc == PICO_ERROR_GENERIC || rc == PICO_ERROR_TIMEOUT)
            len = 0;

        return len;
    }

    //--------------------------------------------------------------------------------------------
    // For the FPC data, the first two bytes are the length of the data to follow. So this method reads in
    // in the length and returns it. This method is the "start" of a FPC data read operation. It doesn't
    // stop/end the I2C read operation, that is done in the readPayload() method.

    uint16_t readTransferSize(uint8_t device_address)
    {
        if (!_isInitialized)
            return 0;

        _device_address = device_address;
        uint16_t theSize = 0;
        int rc = i2c_read_blocking_until(_i2cPort, device_address, (uint8_t *)&theSize, sizeof(theSize), true,
                                         make_timeout_time_ms(_timeOutMillis));

        if (rc == PICO_ERROR_GENERIC || rc == PICO_ERROR_TIMEOUT)
            theSize = 0;
        else
            _pendingStop = true;

        return theSize;
    }

  private:
    uint8_t _device_address;
    i2c_inst_t *_i2cPort;
    bool _isInitialized;
    bool _pendingStop;
    uint16_t _timeOutMillis;
};

#endif