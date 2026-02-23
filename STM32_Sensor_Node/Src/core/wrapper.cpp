/**
 * @file wrapper.cpp
 * @brief C-callable wrapper implementation for the Room instance.
*/

#include <stdint.h>

#include "rooms.h"
#include "wrapper.h"

// Allocate a room
static Room room(101U);

// Sensor setters
void setTemperature(uint16_t value) {
    room.getTemperatureSensor()->setValue(value);
}

void setMotion(uint16_t value) {
    room.getMotionDetector()->setValue(value);
}

// Sensor getters
uint16_t getTemperature(void) {
    return room.getTemperatureSensor()->readValue();
}

uint16_t getMotion(void) {
    return room.getMotionDetector()->readValue();
}   

// Device Control
void turnOnLight(void)   { room.turnOnLight();  }
void turnOffLight(void)  { room.turnOffLight(); }
void turnOnAC(void)      { room.turnOnAC();     }
void turnOffAC(void)     { room.turnOffAC();    }
void turnOnHeater(void)  { room.turnOnHeater(); }
void turnOffHeater(void) { room.turnOffHeater();}