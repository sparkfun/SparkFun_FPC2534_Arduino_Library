/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include "sfDevFPC2534I2C.h"

// platform specific read helpers - a system we support?
#if defined(ESP32)

#include "sfDevFPC2534I2C_esp32.h"
static sfDevFPC2534I2C_Helper __esp32ReadHelper;
static sfDevFPC2534I2C_IRead *__readHelper = &__esp32ReadHelper;

#elif defined(ARDUINO_ARCH_RP2040)
#include "sfDevFPC2534I2C_rp2.h"
static sfDevFPC2534I2C_Helper __rp2040ReadHelper;
static sfDevFPC2534I2C_IRead *__readHelper = &__rp2040ReadHelper;
#else

#warning "No platform specific I2C read helper defined"
static sfDevFPC2534I2C_IRead *__readHelper = nullptr;

#endif

// For the ISR interrupt handler
static volatile bool data_available = false;

static void the_isr_cb()
{
    // This is the interrupt callback function
    // It will be called when the IRQ pin goes high
    // We can use this to signal that data is available
    data_available = true;
}

sfDevFPC2534I2C::sfDevFPC2534I2C() : _i2cAddress{0}, _i2cPort{nullptr}, _i2cBusNumber{0}, _dataLength{0}, _dataOffset{0}
{
}

//--------------------------------------------------------------------------------------------
bool sfDevFPC2534I2C::initialize(uint8_t address, TwoWire &wirePort, uint8_t i2cBusNumber, uint32_t interruptPin)
{
    // do we have a i2c helper ?
    if (__readHelper == nullptr)
        return false;

    __readHelper->initialize(i2cBusNumber);
    _i2cAddress = address;
    _i2cPort = &wirePort;
    _i2cBusNumber = i2cBusNumber;

    // Setup the interrupt handler
    pinMode(interruptPin, INPUT);
    attachInterrupt(interruptPin, the_isr_cb, RISING);

    // The system starts up with a status command available and it might not be caught by the interrupt,
    // So seed this by setting data_available to true.
    //
    // Note: Observation shows that if we don't read the initial status command, no
    // further data is available.
    //
    // TODO: Users can disable this startup status message - if an issue, the user should be able to disable this
    // initial check
    data_available = true;

    return true;
}

//--------------------------------------------------------------------------------------------

bool sfDevFPC2534I2C::dataAvailable()
{
    if (_i2cPort == nullptr)
        return false;

    // bool rc = data_available;
    // data_available = false;
    // return rc;

    // the data available flag is set, or we have data in the buffer
    return data_available || _dataLength > 0;
}

//--------------------------------------------------------------------------------------------
void sfDevFPC2534I2C::clearData()
{
    _dataLength = 0;
    _dataOffset = 0;
}

//--------------------------------------------------------------------------------------------
uint16_t sfDevFPC2534I2C::write(const uint8_t *data, size_t len)
{
    if (_i2cPort == nullptr)
        return FPC_RESULT_IO_RUNTIME_FAILURE; // I2C bus not initialized

    // need to add size of packet to the data stream - it's what is required by the FPC protocol
    uint8_t buffer[len + 2];
    buffer[0] = len & 0xFF;
    buffer[1] = (len >> 8) & 0xFF;
    memcpy(&buffer[2], data, len);

    _i2cPort->beginTransmission(_i2cAddress);

    _i2cPort->write(buffer, sizeof(buffer));

    return _i2cPort->endTransmission() ? FPC_RESULT_FAILURE : FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
uint16_t sfDevFPC2534I2C::read(uint8_t *data, size_t len)
{

    // got port
    if (_i2cPort == nullptr || __readHelper == nullptr)
        return FPC_RESULT_IO_RUNTIME_FAILURE;

    if (_dataLength == 0)
    {

        // At this point, we are reading in data - clear out the data available flag used by interrupt
        data_available = false;
        // read in the packet size.
        _dataLength = __readHelper->readTransferSize(_i2cAddress);

        // Serial.printf("I2C read Packet Size - data size: %d\n\r", (int)_dataLength);
        if (_dataLength == 0)
            return FPC_RESULT_FAILURE;

        _dataLength = __readHelper->readPayload(_dataLength, _dataBuffer);

        // No data returned, no dice
        if (_dataLength == 0)
            return FPC_RESULT_IO_BAD_DATA; // error

        _dataOffset = 0;
    }

    if (_dataLength >= len && len > 0)
    {
        if (data == nullptr)
            return FPC_RESULT_INVALID_PARAM;

        memcpy(data, _dataBuffer + _dataOffset, len);
        _dataLength -= len;
        _dataOffset += len;
    }

    return FPC_RESULT_OK;
}