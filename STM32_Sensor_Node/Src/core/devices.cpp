/**
 * @file devices.cpp
 * @brief Implementation of Device base class and derived device classes.
*/

#include <stdint.h>
#include "devices.h"

/** @brief Device base class Implementation */
Device::Device(uint16_t deviceNumber) 
    : deviceNumber(deviceNumber), deviceState(false) {}

void Device::turnOn() {
    deviceState = true;
}

void Device::turnOff() {
    deviceState = false;
}

bool Device::getState() const {
    return deviceState;
}

/** @brief Light derived class Implementation */
Light::Light(uint16_t deviceNumber) 
    : Device(deviceNumber) {}

/** @brief AC derived class Implementation */
AC::AC(uint16_t deviceNumber) 
    : Device(deviceNumber) {}

/** @brief Heater derived class Implementation */
Heater::Heater(uint16_t deviceNumber) 
    : Device(deviceNumber) {}