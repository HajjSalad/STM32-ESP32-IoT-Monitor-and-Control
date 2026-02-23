#ifndef DEVICES_H_
#define DEVICES_H_

/**
 * @file devices.h
 * @brief Device base class and derived classes declarations
*/

#include <stdint.h>

/** @brief Abstract base class Device for all device types */
class Device {
protected:
    uint16_t deviceNumber;                      // Unique ID for the device
    bool deviceState;                           // Current state of the device (on/off)

public:
    explicit Device(uint16_t deviceNumber);     // Constructor 
    virtual void turnOn();                      // Turn the device on
    virtual void turnOff();                     // Turn the device off
    bool getState() const;                      // Get the current state of the device

    virtual ~Device() = default;                // Virtual destructor for proper cleanup
};

/** @brief Derived class for Light devices */
class Light : public Device {
public:
    explicit Light(uint16_t deviceNumber);
};

/** @brief Derived class for AC devices */
class AC : public Device {
public:
    explicit AC(uint16_t deviceNumber);
};

/** @brief Derived class for Heater devices */
class Heater : public Device {
public:
    explicit Heater(uint16_t deviceNumber);
};

#endif /* DEVICES_H_ */