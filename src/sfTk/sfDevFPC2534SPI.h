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
#include <SPI.h>

// SPI impl for the FPC2534 communication interface

class sfDevFPC2534SPI : public sfDevFPC2534IComm
{
  public:
    sfDevFPC2534SPI();
    bool initialize(SPIClass &spiPort, SPISettings &busSPISettings, uint8_t csPin, uint32_t interruptPin,
                    bool bInit = false);
    bool initialize(uint8_t csPin, uint32_t interruptPin, bool bInit = false);

    bool dataAvailable() override;
    void clearData() override;
    uint16_t write(const uint8_t *data, size_t len) override;
    uint16_t read(uint8_t *data, size_t len) override;

    void beginWrite(void) override;
    void endWrite(void) override;

    void beginRead(void) override;
    void endRead(void) override;

  private:
    bool _inWrite;
    bool _inRead;

    // SPI Things
    SPIClass *_spiPort;
    SPISettings _spiSettings;
    uint8_t _csPin;
};