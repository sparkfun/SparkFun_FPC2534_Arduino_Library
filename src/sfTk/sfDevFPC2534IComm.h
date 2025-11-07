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

#include <stddef.h>
#include <stdint.h>

// Define the communication interface for the FPC2534 fingerprint sensor library

class sfDevFPC2534IComm
{
  public:
    sfDevFPC2534IComm() : _dataAvailable{false}, _usingISRParam{true} {};
    virtual bool dataAvailable(void) = 0;
    virtual void clearData(void) = 0;
    virtual uint16_t write(const uint8_t *data, size_t len) = 0;
    virtual uint16_t read(uint8_t *data, size_t len) = 0;
    // On SPI writes, the CS line needs to remain low during writes (which have multiple blocks).
    // So add a normally no-op beginWrite and endWrite methods that can be overridden by SPI comm classes.
    virtual void beginWrite(void) {};
    virtual void endWrite(void) {};

    // public method -- for the ISR handler to set the data available flag for the specific object
    // representing the IRS callback parameter.
    void setISRDataAvailable(void);

  protected:
    // All communication protocols/types supported by the sensor use an interrupt to signal data availability.
    // This is required for i2c and SPI interfaces (UART is okay b/c of Arduino Serial buffer handling). So
    // we consolidate the core interrupt handling in this case. If needed, comm type specializations can use this.
    void initISRHandler(uint32_t interruptPin);
    bool isISRDataAvailable(void);

    void clearISRDataAvailable(void);

  private:
    volatile bool _dataAvailable;
    bool _usingISRParam;
};
