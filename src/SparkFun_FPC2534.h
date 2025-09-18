

#pragma once

#include "sfTk/sfDevFPC2534.h"
#include "sfTk/sfDevFPC2534I2C.h"
#include "sfTk/sfDevFPC2534UART.h"

// Make a Arduino friendly Address define

#define SFE_FPC2534_I2C_ADDRESS kFPC2534DefaultAddress

//--------------------------------------------------------------------------------------------
class SfeFPC2534I2C : public sfDevFPC2534
{
  public:
    SfeFPC2534I2C()
    {
    }
    bool begin(const uint8_t address = kFPC2534DefaultAddress, TwoWire &wirePort = Wire, const uint8_t i2cBusNumber = 0,
               const uint32_t interruptPin = 255)
    {

        if (!_commI2CBus.initialize(address, wirePort, i2cBusNumber, interruptPin))
            return false;

        // Okay, the bus is a go, lets initialize the base class

        return sfDevFPC2534::initialize(_commI2CBus);
    }

  private:
    sfDevFPC2534I2C _commI2CBus;
};

//--------------------------------------------------------------------------------------------
class SfeFPC2534UART : public sfDevFPC2534
{
  public:
    SfeFPC2534UART()
    {
    }
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