#ifndef TASKS_H
#define TASKS_H

/**
 * @file task_s.h
 * @brief Tasks interface.
*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Macros for consistent log formatting
#define LOG_SENSOR_DATA(msg, taskname, action, temp, motion) \
    snprintf(msg, sizeof(msg), "[%-12s] %-18s Temp: %3u  Motion: %u", \
            taskname, action, (unsigned int)(temp), (unsigned int)(motion))

// Macro for logging transmit data with timestamp
#define LOG_TRANSMIT_DATA(msg,taskname, action, temp, motion, ts) \
    snprintf(msg, sizeof(msg), "[%-12s] %-18s Temp: %3u  Motion: %u  Timestamp: %lu", \
             taskname, action, (unsigned int)(temp), (unsigned int)(motion), (unsigned long)(ts))


void vTaskSensorWrite(void *pvParameters);
void vTaskSensorRead(void *pvParameters);
void vTaskController(void *pvParameters);
void vTaskTransmit(void *pvParameters);
void vTaskLogger(void *pvParameters);

#endif // TASKS_H 