/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include <Arduino.h>
// Implementation file for the I2C communication class of the library.
#include "sfDevFPC2534IComm.h"

// When in I2C comm mode, an interrupt pin from the FPC2534 is used to signal when
// data is available to read. We manage this here.
//
// For the ISR interrupt handler
// static volatile bool data_available = false;
static volatile bool data_available = false;

static bool isISRInitialized = false;

#if !defined(ESP32) && !defined(ARDUINO_ARCH_RP2040)
//--------------------------------------------------------------------------------------------
// standard ISR handler - no param version
static void the_isr_cb()
{
    // This is the interrupt callback function
    // It will be called when the IRQ pin goes high
    // We can use this to signal that data is available

    data_available = true;
}
#else
//--------------------------------------------------------------------------------------------
// ISR handler with param version ()
static void the_isr_cb_arg(void *arg)
{
    // set the data available flag in the instance
    if (arg != nullptr)
        static_cast<sfDevFPC2534IComm *>(arg)->setISRDataAvailable();
}
#endif
//--------------------------------------------------------------------------------------------
// method used to set the IRS Handler by a sub-class
void sfDevFPC2534IComm::initISRHandler(uint32_t interruptPin)
{
    // Some platforms (ESP32 , RP2040) support passing an argument to the ISR handler.
    // If so, use that method to pass in "this" pointer to the handler. Which allows
    // us to set the data_available flag in the instance, rather than a static/global flag
    // and possibly support multiple sensors at the same time.

    pinMode(interruptPin, INPUT);
#if defined(ESP32)

    attachInterruptArg(interruptPin, the_isr_cb_arg, (void *)this, RISING);
    _usingISRParam = true;

#elif defined(ARDUINO_ARCH_RP2040)

    attachInterruptParam(interruptPin, the_isr_cb_arg, RISING, (void *)this);
    _usingISRParam = true;

#else

    // Setup the interrupt handler for non-parameter version (older arduino impls)
    attachInterrupt(interruptPin, the_isr_cb, RISING);

    isISRInitialized = true;
    _usingISRParam = false;
#endif
}
//--------------------------------------------------------------------------------------------
void sfDevFPC2534IComm::setISRDataAvailable(void)
{
    _dataAvailable = true;
}
//--------------------------------------------------------------------------------------------
// method used to clear the data available flag
void sfDevFPC2534IComm::clearISRDataAvailable(void)
{
    // Are we using the ISR param method?
    if (_usingISRParam)
        _dataAvailable = false;
    else if (isISRInitialized)
        data_available = false;
}

//--------------------------------------------------------------------------------------------
// Data available ?
bool sfDevFPC2534IComm::isISRDataAvailable(void)
{
    // Serial.printf("isISRDataAvailable: usingISRParam=%d, _dataAvailable=%d\r\n", _usingISRParam, _dataAvailable);
    // Are we using the ISR param method?
    if (_usingISRParam)
        return _dataAvailable;

    // Nope, using the static ISR and static flag in this file (this only supports one instance)
    if (!isISRInitialized)
        return false;

    return data_available;
}