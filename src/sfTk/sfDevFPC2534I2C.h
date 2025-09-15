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

// Define an interface to peform the needed read actions for the I2C protocol - this is needed since
// the FPC2534 uses a custom I2C read protocol that the standard Arduino Wire library does not support.

class sfDevFPC2534I2C_IRead
{
  public:
    virtual void initialize(uint8_t i2cBusNumber) = 0;
    virtual uint16_t readPayload(size_t len, uint8_t *data) = 0;
    virtual uint16_t readTransferSize(uint8_t device_address) = 0;
}

// i2c impl for the FPC2534 communication class

class sfDevFPC2534I2C : public sfDevFPC2534IComm
{
  public:
    sfDevFPC2534I2C();
    bool initialize(uint8_t address, Wire &wirePort = Wire, uint8_t i2cBusNumber, uint32_t interruptPin);
    bool dataAvailable();
    void clearData();
    uint16_t write(const uint8_t *data, size_t len);
    uint16_t read(uint8_t *data, size_t len);

  private:
    uint8_t _i2cAddress;
    TwoWire *_i2cPort;
    uint8_t _i2cBusNumber;

    // Internal data buffer items
    static constexpr size_t kDataBufferSize = 2048;

    uint16_t _dataLength;
    uint16_t _dataOffset;
    uint8_t _dataBuffer[kDataBufferSize];
};