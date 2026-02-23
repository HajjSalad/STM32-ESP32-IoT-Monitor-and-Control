/**
 * @file task_sensor.c
 * @brief Sensor data simulation task.
 * 
 * Simulates sensor readings using rand() and writes them into the Room object via the C wrapper interface.
 * In a real application, this would be replaced with actual hardware peripheral reads.
 * Sends log messages to the logger task via a FreeRTOS queue.
*/

#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "wrapper.h"
#include "shared_resources.h"
#include "task_logger.h"
#include "task_sensor_write.h"

#define SENSOR_TASK_PERIOD_MS    (10000U)
#define SENSOR_TEMP_MAX          (100U)
#define SENSOR_MOTION_MAX        (2U)

/**
 * @brief Sensor simulation task entry point.
 * 
 * Periodically generates simulated temperature and motion values,
 * writes them to the Room object, and logs the results.
 * 
 * @param pvParameters Unused parameter required by FreeRTOS task signature.
*/
void vTaskSensorWrite(void *pvParameters)
{
    (void)pvParameters;                 // Suppress unused parameter warning

    char       msg[LOG_MSG_MAX_LEN];
    BaseType_t xRet         = pdFALSE;
    uint16_t   usTempValue   = 0U;
    uint16_t   usMotionValue = 0U;

    while (1) 
    {
        // Simulate sensor readings
        usTempValue   = (uint16_t)(rand() % (int)SENSOR_TEMP_MAX);       // 0-99
        usMotionValue = (uint16_t)(rand() % (int)SENSOR_MOTION_MAX);     // 0 or 1

        // Write to Room object via C wrapper
        xSemaphoreTake(xSensorMutex, portMAX_DELAY);        // Take the mutex
        setTemperature(usTempValue);
        setMotion(usMotionValue);
        xRet = xSemaphoreGive(xSensorMutex);                // Release the mutex
        configASSERT(xRet == pdTRUE);                       // Ensure mutex was released successfully

        // Log written values
        LOG_SENSOR_DATA(msg, "SensorWrite", "Set sensor values:", usTempValue, usMotionValue);
        xRet = xQueueSend(xLogQueue, (const void *)msg, 0U);
        if (xRet != pdTRUE) {
            /* Log queue full — drop message */
        }

        // Sleep until next period
        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
    }
}