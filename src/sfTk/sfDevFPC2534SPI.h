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
#include <SPI.h>

// SPI impl for the FPC2534 communication interface

class sfDevFPC2534SPI : public sfDevFPC2534IComm
{
  public:
    sfDevFPC2534SPI();
    bool initialize(SPIClass &spiPort, SPISettings &busSPISettings, uint8_t csPin, uint32_t interruptPin,
                    bool bInit = false);
    bool initialize(uint8_t csPin, uint32_t interruptPin, bool bInit = false);

    bool dataAvailable();
    void clearData();
    uint16_t write(const uint8_t *data, size_t len);
    uint16_t read(uint8_t *data, size_t len);

  private:
    // SPI Things
    SPIClass *_spiPort;
    SPISettings _spiSettings;
    uint8_t _csPin;
};