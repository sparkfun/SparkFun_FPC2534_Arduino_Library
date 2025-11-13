/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include "sfDevFPC2534SPI.h"

// --------------------------------------------------------------------------------------------
// CTOR
sfDevFPC2534SPI::sfDevFPC2534SPI() : _inWrite{false}, _spiPort{nullptr}, _csPin{0}
{
}

//--------------------------------------------------------------------------------------------
bool sfDevFPC2534SPI::initialize(SPIClass &spiPort, SPISettings &busSPISettings, uint8_t csPin, uint32_t interruptPin,
                                 bool bInit)

{
    _spiPort = &spiPort;

    _spiSettings = busSPISettings;
    _csPin = csPin;

    // Call our super to init the ISR handler
    sfDevFPC2534IComm::initISRHandler(interruptPin);

    // clear out our data buffer
    clearData();

    // init?
    if (bInit)
        _spiPort->begin();

    return true;
}
//--------------------------------------------------------------------------------------------
bool sfDevFPC2534SPI::initialize(uint8_t csPin, uint32_t interruptPin, bool bInit)
{
    // If the transaction settings are not provided by the user they are built here.
    SPISettings spiSettings = SPISettings(3000000, MSBFIRST, SPI_MODE0);
    return initialize(SPI, spiSettings, csPin, interruptPin, bInit);
}
//--------------------------------------------------------------------------------------------
// Is data available to read - either the device is indicating it via an interrupt, or we have
// data in our internal buffer
//
bool sfDevFPC2534SPI::dataAvailable()
{
    if (_spiPort == nullptr)
        return false;

    // the data available flag is set, or we have data in the buffer
    return isISRDataAvailable();
}

//--------------------------------------------------------------------------------------------
// Clear out the internal data buffer...
//
void sfDevFPC2534SPI::clearData()
{
    // clear any data signaled by the ISR
    clearISRDataAvailable();
}

void sfDevFPC2534SPI::beginWrite(void)
{

    if (_spiPort == nullptr)
        return; // SPI bus not initialized

    _spiPort->beginTransaction(_spiSettings);

    // Signal communication start
    digitalWrite(_csPin, LOW);
    // the  datasheet specifiies a delay greater than 500us after CS goes low
    delayMicroseconds(600);
    _inWrite = true;
    Serial.println("Begin SPI Write");
}

void sfDevFPC2534SPI ::endWrite(void)
{
    if (_spiPort == nullptr)
        return; // SPI bus not initialized
    // End comms
    digitalWrite(_csPin, HIGH);
    _spiPort->endTransaction();
    _inWrite = false;
    Serial.println("End SPI Write");
}
//--------------------------------------------------------------------------------------------
// Write data to the device
//
uint16_t sfDevFPC2534SPI::write(const uint8_t *data, size_t len)
{
    if (_spiPort == nullptr)
        return FPC_RESULT_IO_RUNTIME_FAILURE; // I2C bus not initialized

    Serial.printf("Writing %d bytes to SPI\r\n", len);
    // now send the data

    for (size_t i = 0; i < len; i++)
        _spiPort->transfer(*data++);

    return FPC_RESULT_OK;
}

//--------------------------------------------------------------------------------------------
uint16_t sfDevFPC2534SPI::read(uint8_t *data, size_t len)
{
    // got port
    if (_spiPort == nullptr)
        return FPC_RESULT_IO_RUNTIME_FAILURE;

    if (len == 0)
        return FPC_RESULT_OK;

    // lets return data to the user...
    if (data == nullptr)
        return FPC_RESULT_INVALID_PARAM;

    if (_inWrite)
        endWrite();

    _spiPort->beginTransaction(_spiSettings);

    // Signal communication start
    digitalWrite(_csPin, LOW);

    // the  datasheet specifiies a delay greater than 500us after CS goes low
    delayMicroseconds(600);

    Serial.printf("Reading %d bytes from SPI\r\n", len);
    // Lets read the data...
    for (size_t i = 0; i < len; i++)
        *data++ = _spiPort->transfer(0x00);

    // End transaction
    digitalWrite(_csPin, HIGH);
    _spiPort->endTransaction();

    return FPC_RESULT_OK;
}
