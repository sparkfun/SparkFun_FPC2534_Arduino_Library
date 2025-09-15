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

#include <stdint.h>

// Define the communication interface for the FPC2534 fingerprint sensor library

class sfDevFPC2534IComm
{
  public:
    virtual bool dataAvailable(void) = 0;
    virtual void clearData(void) = 0;
    virtual uint16_t write(const uint8_t *data, size_t len) = 0;
    virtual uint16_t read(uint8_t *data, size_t len) = 0;
};