#ifndef ROOMS_H
#define ROOMS_H

/**
 * @file  rooms.h
 * @brief Room base class declarations.
*/

#include <stdint.h>
#include "devices.h"
#include "sensors.h"

/** @brief Abstract base class Room */
class Room {
protected:
    uint16_t roomNumber;                      // Unique ID for the room

    // Sensors and devices in a room
    MotionDetector     motionDetector;
    TemperatureSensor  tempSensor;
    Light              light;
    AC                 ac;
    Heater             heater;

public:
    explicit Room(uint16_t roomNumber);        // Constructor
    
    // Public interface to access room's sensors and devices

    // Const versions for read-only access
    const MotionDetector* getMotionDetector() const;
    const TemperatureSensor* getTemperatureSensor() const;
    const Light* getLight() const;
    const AC* getAC() const;
    const Heater* getHeater() const;

    // Non-const versions for write access
    MotionDetector* getMotionDetector();
    TemperatureSensor* getTemperatureSensor();
    Light* getLight();
    AC* getAC();
    Heater* getHeater();
    
    // Methods to control devices in the room
    void turnOnLight();
    void turnOffLight();
    void turnOnAC();
    void turnOffAC();
    void turnOnHeater();
    void turnOffHeater();

    virtual ~Room() = default;                // Virtual destructor for proper cleanup
};

#endif /* ROOMS_H */