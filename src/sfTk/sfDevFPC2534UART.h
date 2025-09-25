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

#include <Arduino.h>

#include "sfDevFPC2534IComm.h"

// TODO: With the RP2350/RP2040 port, a "arduino::" namespace is needed for HardwareSerial I belieave.
//       Note for future port

// uart impl for the FPC2534 communication class

class sfDevFPC2534UART : public sfDevFPC2534IComm
{
  public:
    sfDevFPC2534UART();
    bool initialize(HardwareSerial &theUART);
    bool dataAvailable(void);
    void clearData(void);
    uint16_t write(const uint8_t *data, size_t len);
    uint16_t read(uint8_t *data, size_t len);

  private:
    HardwareSerial *_theUART;
};