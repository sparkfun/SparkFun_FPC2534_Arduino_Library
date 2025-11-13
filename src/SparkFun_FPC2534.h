
/**
 * @file SparkFun_FPC2534.h
 * @brief Arduino library header for the SparkFun FPC2534 Fingerprint Sensor
 *
 * This file contains the Arduino friendly wrapper for the FPC2534 library functionality.
 * Separate classes are provided for each communication protocol available for the sensor.
 * Currently this includes I2C and Serial/UART.
 *
 * Actual logic and implementation is in the sfDevFPC2534 class and related files.
 *
 * @author SparkFun Electronics
 * @date   2025
 * @copyright Copyright (c) 2025 SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "sfTk/sfDevFPC2534.h"
#include "sfTk/sfDevFPC2534I2C.h"
#include "sfTk/sfDevFPC2534SPI.h"
#include "sfTk/sfDevFPC2534UART.h"
#include <Arduino.h>

// Make a Arduino friendly Address define
#define SFE_FPC2534_I2C_ADDRESS kFPC2534DefaultAddress

//--------------------------------------------------------------------------------------------
// I2C version of the FPC2534 class
//
///

class SfeFPC2534I2C : public sfDevFPC2534
{
  public:
    SfeFPC2534I2C()
    {
    }
    /**
     * @brief Initialize the sensor using I2C communication
     *
     * @param address I2C address of the sensor (default is kFPC2534DefaultAddress)
     * @param wirePort Reference to the TwoWire object to use (default is Wire)
     * @param i2cBusNumber I2C bus number (default is 0) This should match the bus number used in the Wire object
     * @param interruptPin Pin number for the interrupt (default is 255, meaning no interrupt)
     * @return true if initialization was successful, false otherwise
     */

    bool begin(const uint8_t address = kFPC2534DefaultAddress, TwoWire &wirePort = Wire, const uint8_t i2cBusNumber = 0,
               const uint32_t interruptPin = 255)
    {

        // Setup the I2C communication
        if (!_commI2CBus.initialize(address, wirePort, i2cBusNumber, interruptPin))
            return false;

        // Okay, the bus is a go, lets initialize the base class

        return sfDevFPC2534::initialize(_commI2CBus);
    }

  private:
    sfDevFPC2534I2C _commI2CBus;
};

//--------------------------------------------------------------------------------------------
// UART/Serial version of the FPC2534 class
//
class SfeFPC2534UART : public sfDevFPC2534
{
  public:
    SfeFPC2534UART()
    {
    }
    /**
     * @brief Initialize the sensor using UART communication
     *
     * @param theUART Reference to the HardwareSerial object to use
     * @return true if initialization was successful, false otherwise
     */
    bool begin(HardwareSerial &theUART)
    {

        if (!_commUART.initialize(theUART))
            return false;

        // Okay, the bus is a go, lets initialize the base class

        return sfDevFPC2534::initialize(_commUART);
    }

  private:
    sfDevFPC2534UART _commUART;
};

//--------------------------------------------------------------------------------------------
// SPI version of the FPC2534 class
//
///

class SfeFPC2534SPI : public sfDevFPC2534
{
  public:
    SfeFPC2534SPI()
    {
    }
    /**
     * @brief Initialize the sensor using SPI communication
     * @param spiPort Reference to the SPIClass object to use (default is SPI)
     * @param busSPISettings SPI settings to use for the bus
     * @param csPin Chip select pin number
     * @param interruptPin Pin number for the interrupt)
     * @param bInit Whether to initialize the SPI bus (default is false)
     * @return true if initialization was successful, false otherwise
     */

    bool begin(SPIClass &spiPort, SPISettings &busSPISettings, const uint8_t csPin, const uint32_t interruptPin,
               bool bInit = false)
    {

        // Setup the SPI communication
        if (!_commSPIBus.initialize(spiPort, busSPISettings, csPin, interruptPin, bInit))
            return false;

        // Okay, the bus is a go, lets initialize the base class

        return sfDevFPC2534::initialize(_commSPIBus);
    }

    /**
     * @brief Initialize the sensor using SPI communication
     *
     * @param csPin Chip select pin number
     * @param interruptPin Pin number for the interrupt (default is 255, meaning no interrupt)
     * @param bInit Whether to initialize the SPI bus (default is false)
     * @return true if initialization was successful, false otherwise
     */
    bool begin(const uint8_t csPin, const uint32_t interruptPin, bool bInit = false)
    {

        // Setup the SPI communication
        if (!_commSPIBus.initialize(csPin, interruptPin, bInit))
            return false;

        // Okay, the bus is a go, lets initialize the base class

        return sfDevFPC2534::initialize(_commSPIBus);
    }

  private:
    sfDevFPC2534SPI _commSPIBus;
};