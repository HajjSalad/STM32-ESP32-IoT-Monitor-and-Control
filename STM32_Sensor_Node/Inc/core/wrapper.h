#ifndef WRAPPER_H
#define WRAPPER_H

/**
 * @file wrapper.h
 * @brief C-callable wrapper interface for the Room instance.
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sensor setters
void setTemperature(uint16_t value);
void setMotion(uint16_t value);

// Sensor getters
uint16_t getTemperature(void);
uint16_t getMotion(void);

// Device Control
void turnOnLight(void);
void turnOffLight(void);
void turnOnAC(void);
void turnOffAC(void);
void turnOnHeater(void);
void turnOffHeater(void);

#ifdef __cplusplus
}
#endif

#endif // WRAPPER_H