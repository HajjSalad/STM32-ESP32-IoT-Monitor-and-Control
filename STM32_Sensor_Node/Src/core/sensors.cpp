/**
 * @file sensors.cpp
 * @brief Implementation of Sensor base class and derived sensor classes.
*/

#include <stdint.h>
#include "sensors.h"

/** @brief Sensor base class Implementation */
Sensor::Sensor(uint16_t sensorNumber) 
    : sensorNumber(sensorNumber), sensorValue(0U) {}

void Sensor::setValue(uint16_t value) {
    sensorValue = value;
}

/** @brief MotionDetector derived class Implementation */
MotionDetector::MotionDetector(uint16_t sensorNumber) 
    : Sensor(sensorNumber) {}

uint16_t MotionDetector::readValue() {
    return sensorValue;
}

/** @brief TemperatureSensor derived class Implementation */
TemperatureSensor::TemperatureSensor(uint16_t sensorNumber) 
    : Sensor(sensorNumber) {}

uint16_t TemperatureSensor::readValue() {
    return sensorValue;
}