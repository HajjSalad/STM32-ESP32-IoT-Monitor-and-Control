#ifndef SENSORS_H_
#define SENSORS_H_

/**
 * @file sensors.h
 * @brief Sensor base class and derived classes declarations
*/

#include <stdint.h>

/** @brief Abstract base class Sensor for all sensor types */
class Sensor {
protected:
    uint16_t sensorNumber;                      // Unique ID for the sensor
    uint16_t sensorValue;                       // Current sensor reading   

public:
    explicit Sensor(uint16_t sensorNumber);     // Constructor 
    virtual uint16_t readValue() = 0;           // Read sensor value
    void setValue(uint16_t value);              // Set sensor value

    virtual ~Sensor() = default;                // Virtual destructor for proper cleanup
};

/** @brief Motion detector sensor */
class MotionDetector : public Sensor {
public:
    explicit MotionDetector(uint16_t sensorNumber);
    uint16_t readValue() override;             
};

/** @brief Temperature sensor */
class TemperatureSensor : public Sensor {
public:
    explicit TemperatureSensor(uint16_t sensorNumber);
    uint16_t readValue() override;
};

#endif /* SENSORS_H_ */