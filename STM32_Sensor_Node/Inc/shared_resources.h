#ifndef SHARED_RESOURCES_H
#define SHARED_RESOURCES_H

/**
 * @file shared_resources.h
 * @brief Shared resources for FreeRTOS tasks.
*/

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"

#define LOG_MSG_MAX_LEN         (128U)
#define LOG_QUEUE_DEPTH         (20U)
#define SENSOR_QUEUE_DEPTH      (20U)
#define STREAM_BUFFER_SIZE      ((size_t) 256U)

/**
 * @brief Sensor data structure.
 * 
 * Contains the latest temperature and motion readings from the Room's sensors.
 * Transmitted via xSensorQueue from vTaskSensorRead to vTaskController.
*/
typedef struct {
    uint16_t temperature;   /**< Temperature sensor reading */
    uint16_t motion;        /**< Motion detector value */
} SensorData_t;

/**
 * @brief Transmission data structure sent to ESP32.
 * 
 * Extends SensorData_t with a timestamp for event correlation and data logging.
 * Written to xStreamBuffer by vTaskController, read by vTaskTransmit.
*/
typedef struct {
    uint16_t temperature;
    uint16_t motion;
    TickType_t timestamp;
} TransmitData_t;

// Global resource handles
extern SemaphoreHandle_t    xSensorMutex;
extern QueueHandle_t        xLogQueue;
extern QueueHandle_t        xSensorQueue;
extern StreamBufferHandle_t xStreamBuffer;

#endif // SHARED_RESOURCES_H