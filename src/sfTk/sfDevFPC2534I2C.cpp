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

// platform specific read helpers - a system we support....
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

// When in I2C comm mode, an interrupt pin from the FPC2534 is used to signal when
// data is available to read. We manage this here.
//
// For the ISR interrupt handler
// static volatile bool data_available = false;
static volatile bool data_available = false;

static void the_isr_cb()
{
    // This is the interrupt callback function
    // It will be called when the IRQ pin goes high
    // We can use this to signal that data is available

    data_available = true;
}

// --------------------------------------------------------------------------------------------
// CTOR
sfDevFPC2534I2C::sfDevFPC2534I2C() : _i2cAddress{0}, _i2cPort{nullptr}, _i2cBusNumber{0}
{
}

//--------------------------------------------------------------------------------------------
bool sfDevFPC2534I2C::initialize(uint8_t address, TwoWire &wirePort, uint8_t i2cBusNumber, uint32_t interruptPin)
{
    // do we have a i2c helper ?
    if (__readHelper == nullptr)
        return false;

    // Initialize the I2C read helper - pass in the bus number being used ...
    __readHelper->initialize(i2cBusNumber);
    _i2cAddress = address;
    _i2cPort = &wirePort;
    _i2cBusNumber = i2cBusNumber;

    // Setup the interrupt handler
    pinMode(interruptPin, INPUT);
    attachInterrupt(interruptPin, the_isr_cb, RISING);

    // clear out our data buffer
    clearData();
    return true;
}

//--------------------------------------------------------------------------------------------
// Is data available to read - either the device is indicating it via an interrupt, or we have
// data in our internal buffer
//
bool sfDevFPC2534I2C::dataAvailable()
{
    if (_i2cPort == nullptr)
        return false;

    // the data available flag is set, or we have data in the buffer
    return data_available || _dataCount > 0;
}

//--------------------------------------------------------------------------------------------
// Clear out the internal data buffer...
//
void sfDevFPC2534I2C::clearData()
{
    _dataCount = 0;
    _dataHead = 0;
    _dataTail = 0;
    data_available = false;
}

//--------------------------------------------------------------------------------------------
// Write data to the device
//
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
// Add data to our internal "circular"/FIFO buffer.
bool sfDevFPC2534I2C::fifo_enqueue(uint8_t *data, size_t len)
{
    // adding 0 bytes is success!
    if (len == 0)
        return true;

    // len + null data pointer - no joy
    if (data == nullptr)
        return false;

    // is there room for this?
    if (_dataCount + len >= kDataBufferSize)
        return false;

    // add the data to the buffer
    for (uint16_t i = 0; i < len; i++)
    {
        _dataBuffer[_dataHead] = data[i];
        _dataHead = (_dataHead + 1) % kDataBufferSize;
        _dataCount++;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
// Remove data from the internal FIFO buffer
bool sfDevFPC2534I2C::fifo_dequeue(uint8_t *data, size_t len)
{
    if (len == 0)
        return true;

    // valid data pointer and enough data? (no partial reads)
    if (data == nullptr || _dataCount < len)
        return false;

    for (uint16_t i = 0; i < len; i++)
    {
        data[i] = _dataBuffer[_dataTail];
        _dataTail = (_dataTail + 1) % kDataBufferSize;
        _dataCount--;
    }
    return true;
}

//--------------------------------------------------------------------------------------------
uint16_t sfDevFPC2534I2C::read(uint8_t *data, size_t len)
{

    // got port
    if (_i2cPort == nullptr || __readHelper == nullptr)
        return FPC_RESULT_IO_RUNTIME_FAILURE;

    // is new data available from the sensor - always grab new data if we have room
    if (data_available)
    {
        // clear flag
        data_available = false;

        // how much data is available?
        uint16_t dataAvailable = __readHelper->readTransferSize(_i2cAddress);

        if (dataAvailable > 0)
        {
            uint8_t tempBuffer[dataAvailable];
            dataAvailable = __readHelper->readPayload(dataAvailable, tempBuffer);

            // Was there an error
            if (dataAvailable == 0)
                return FPC_RESULT_IO_BAD_DATA; // error

            // okay, add this data to our internal buffer
            if (fifo_enqueue(tempBuffer, (size_t)dataAvailable) == false)
                return FPC_RESULT_IO_BAD_DATA;
        }
    }

    // lets return data to the user...
    if (data == nullptr)
        return FPC_RESULT_INVALID_PARAM;

    if (len == 0)
        return FPC_RESULT_OK;

    // do we have enough data?
    if (len > _dataCount)
        return FPC_RESULT_IO_NO_DATA;

    if (fifo_dequeue(data, len) == false)
        return FPC_RESULT_IO_BAD_DATA;

    return FPC_RESULT_OK;
}
