/**
 * @file  task_1_sensor_sample.c
 * @brief Sensor acquisition task
 * 
 * Wakes periodically via software timer, reads TMP102 temperature sensor and
 * GPIO Input button motion detector, and writes the values into the Room object
 * via the C wrapper interface
 *
 * Sends log messages to the logger task via a FreeRTOS queue.
*/

#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "tasks.h"
#include "wrapper.h"
#include "sensor_drivers.h"
#include "shared_resources.h"

#define SENSOR_SAMPLE_TASK_PERIOD_MS    (10000U)
#define SENSOR_TEMP_MAX                  (100U)
#define SENSOR_MOTION_MAX                (2U)

xTimerHandle xSampleTimer = NULL;
TaskHandle_t xSensorSampleHandle = NULL;

volatile uint8_t task1_alive = 0U;

/**
 * @brief Software timer callback
 * 
 * Called by the FreeRTOS timer daemon task. Sends a notification to
 * vTaskSensorSample to begin a new sensor acquisition cycle.
*/
void vSampleTimerCallback(TimerHandle_t xTimer) 
{
    // Notify vTaskSensorSample
    xTaskNotifyGive(xSensorSampleHandle);
}

/**
 * @brief Sensor acquisition task entry point.
 * 
 * Blocks on a task notification from the software timer callback. On each
 * wakeup, reads the TMP102 temperature sensor over I2C and the PIR motion
 * detector via GPIO, writes the values into the Room object via the C wrapper
 * interface, and logs the results.
 * 
 * @param pvParameters Unused parameter required by FreeRTOS task signature.
*/
void vTaskSensorSample(void *pvParameters)
{
    (void)pvParameters;                 // Suppress unused parameter warning

    BaseType_t xRet = pdFALSE;
    char msg[LOG_MSG_MAX_LEN];

    uint16_t   usTempValue   = 0U;
    uint16_t   usMotionValue = 0U;

    uint32_t   tick_count    = 0U;

    while (1) 
    {
        // Block until software timer fires
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // 1. Simulate sensor readings
        usTempValue   = (uint16_t)(rand() % (int)SENSOR_TEMP_MAX);       // 0-99
        usMotionValue = (uint16_t)(rand() % (int)SENSOR_MOTION_MAX);     // 0 or 1

        // 2. Write to Room object via C wrapper
        xSemaphoreTake(xSensorMutex, portMAX_DELAY);        // Take the mutex
        setTemperature(usTempValue);
        setMotion(usMotionValue);
        xRet = xSemaphoreGive(xSensorMutex);                // Release the mutex
        configASSERT(xRet == pdTRUE);                       // Ensure mutex was released successfully

        // 3. Notify vTaskSensorRead
        xTaskNotifyGive(xSensorReadHandle);

        // Log written values
        // LOG_SENSOR_DATA(msg, "SensorSample", "Set sensor values:", usTempValue, usMotionValue);
        // xRet = xQueueSend(xLogQueue, (const void *)msg, 0U);
        // if (xRet != pdTRUE) {
        //     /* Log queue full — drop message */
        // }

        // Log HighwaterMark
        if (tick_count++ % 100 == 0) {
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            LOG("SensorSample stack watermark: %u words remaining", watermark);
        }

        // 4. Set alive flag
        task1_alive = 1;
        snprintf(msg, sizeof(msg), "[T1] Sent alive heartbeat");
        xRet = xQueueSend(xLogQueue, msg, 0U);
        if (xRet != pdTRUE) {
            /* Log queue full — increment error counter or set error flag */
        }

        // Sleep until next period
        vTaskDelay(pdMS_TO_TICKS(SENSOR_SAMPLE_TASK_PERIOD_MS));
    }
}