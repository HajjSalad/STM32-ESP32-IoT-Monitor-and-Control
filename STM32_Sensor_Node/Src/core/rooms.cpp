/**
 * @file rooms.cpp
 * @brief Implementation of Room base class and derived room classes.
*/

#include <stdint.h>
#include "rooms.h"

/** @brief Room base class Implementation */
Room::Room(uint16_t roomNumber) 
    : roomNumber(roomNumber),
      motionDetector(roomNumber),
      tempSensor(roomNumber),
      light(roomNumber),
      ac(roomNumber),
      heater(roomNumber){}

const MotionDetector* Room::getMotionDetector() const {
    return &motionDetector;
}

const TemperatureSensor* Room::getTemperatureSensor() const {
    return &tempSensor;
}

const Light* Room::getLight() const {
    return &light;
}   

const AC* Room::getAC() const {
    return &ac;
}

const Heater* Room::getHeater() const {
    return &heater;
}

MotionDetector* Room::getMotionDetector() {
    return &motionDetector;
}

TemperatureSensor* Room::getTemperatureSensor() {
    return &tempSensor;
}

Light* Room::getLight() {
    return &light;
}

AC* Room::getAC() {
    return &ac;
}

Heater* Room::getHeater() {
    return &heater;
}

void Room::turnOnLight() {
    light.turnOn();
}

void Room::turnOffLight() {
    light.turnOff();
}   

void Room::turnOnAC() {
    ac.turnOn();
}

void Room::turnOffAC() {
    ac.turnOff();
}

void Room::turnOnHeater() {
    heater.turnOn();
}

void Room::turnOffHeater() {
    heater.turnOff();
}

