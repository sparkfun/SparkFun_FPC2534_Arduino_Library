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

#include "sfDevFPC2534I2C.h
// ESP32 implementation for the FPC2534 I2C communication class - read protocol.

class sfDevFPC2534I2C_ESP32 : public sfDevFPC2534I2C_IRead
{
  public:
    sfDevFPC2534I2C_ESP32() : _i2cBusNumber{0}, _isInitialized{false}, _pendingStop{false}, _timeoutMillis{50}
    {
    }
    void initialize(uint8_t i2cBusNumber)
    {
        _i2cBusNumber = i2cBusNumber;
        _isInitialized = true;
        _pendingStop = false;
    }
    uint16_t readPayload(size_t len, uint8_t *data);
    uint16_t readTransferSize(uint8_t device_address);

  private:
    uint8_t _i2cBusNumber;
    bool _isInitialized;
    bool _pendingStop;
    uint16_t _timeOutMillis;
}