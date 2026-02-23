/**
 * @file task_sensor_read.c
 * @brief Sensor data reading task.
 * 
 * Reads sensor data from the Room object via the C wrapper interface,
 * logs the values, packages them into a SensorData_t struct, and sends
 * the struct to the Sensor Queue for the controller task to consume.
*/

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "wrapper.h"
#include "shared_resources.h"
#include "task_logger.h"
#include "task_sensor_read.h"

#define SENSOR_READ_TASK_PERIOD_MS    (10000U)

/**
 * @brief Sensor read task entry point.
 *
 * Periodically reads sensor values from the Room object (mutex-protected),
 * logs them, packages them into a struct, and sends to the controller task.
*/
void vTaskSensorRead(void *pvParameters)
{
    (void)pvParameters;                 // Suppress unused parameter warning

    char       msg[LOG_MSG_MAX_LEN];
    BaseType_t xRet          = pdFALSE;
    uint16_t   usTempValue   = 0U;
    uint16_t   usMotionValue = 0U;
    SensorData_t sensorData  = {0U};

    while (1) 
    {
        // Read from Room object via C wrapper
        xSemaphoreTake(xSensorMutex, portMAX_DELAY);        // Take the mutex
        usTempValue = getTemperature();
        usMotionValue = getMotion();
        xRet = xSemaphoreGive(xSensorMutex);                // Release the mutex
        configASSERT(xRet == pdTRUE);                       // Ensure mutex was released successfully

        // Log read values
        LOG_SENSOR_DATA(msg, "SensorRead", "Get sensor values:", usTempValue, usMotionValue);
        xRet = xQueueSend(xLogQueue, (const void *)msg, 0U);
        if (xRet != pdTRUE) {
            /* Log queue full — drop message */
        }

        // Package sensor data into struct 
        sensorData.temperature = usTempValue;
        sensorData.motion      = usMotionValue;

        // Send to controller task via Sensor Queue
        xRet = xQueueSend(xSensorQueue, &sensorData, 0U);
        if (xRet != pdTRUE) {
            /* Sensor queue full — drop data */
        }

        // Sleep until next read cycle
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_TASK_PERIOD_MS));
    }

}