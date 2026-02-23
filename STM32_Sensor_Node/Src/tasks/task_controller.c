/**
 * @file task_controller.c
 * @brief Controller task implementation.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "wrapper.h"
#include "tasks.h"
#include "shared_resources.h"

// Local function prototype
static void control_devices(uint16_t temperature, uint16_t motion);
static void log_messages(const char* taskname, const char* message);

/**
 * @brief Controller task entry point.
 * 
 * This task performs the following:
 * 1. Blocks waiting for sensor data struct from Sensor Queue.
 * 2. Makes control decisions based on sensor values (e.g., turn devices on/off).
 * 3. Writes the sensor data along with a timestamp to a stream buffer for the transmission
 *    task to read and transmit.
 * 4. Logs the transmitted sensor data to the Logger Queue.
 * 
 * @param pvParameters Unused parameter required by FreeRTOS task signature.
*/
void vTaskController(void *pvParameters)
{
    (void)pvParameters;                 // Suppress unused parameter warning

    char            msg[LOG_MSG_MAX_LEN];
    BaseType_t      xRet          = pdFALSE;
    SensorData_t    sensorData    = {0U};
    TransmitData_t  txData        = {0U};
    size_t          bytesWritten  = 0U;

    while (1) 
    {
        // 1. Block waiting for sensor data struct from Sensor Queue
        xRet = xQueueReceive(xSensorQueue, &sensorData, portMAX_DELAY);
        if (xRet != pdTRUE) {
            continue;
        }

        // 2. Make control decision - Turn devices on/off based on sensor values
        control_devices(sensorData.temperature, sensorData.motion);

        // Attach timestamp to transmit data struct
        // For simplicity, we'll just log the current tick count as a timestamp
        txData.temperature = sensorData.temperature;
        txData.motion      = sensorData.motion;
        txData.timestamp   = xTaskGetTickCount();           

        // 3. Write to stream buffer for transmission task
        bytesWritten = xStreamBufferSend(xStreamBuffer, 
                                         &txData, 
                                         sizeof(txData), 
                                         0U);
        if (bytesWritten != sizeof(txData)) {
            // Handle stream buffer send error (e.g., buffer full, log error, set flag)
        }

        // 4. Log the transmitted sensor data
        LOG_TRANSMIT_DATA(msg, "Controller", "Send to stream:", txData.temperature, txData.motion, txData.timestamp);
        xRet = xQueueSend(xLogQueue, msg, 0);
        if (xRet != pdTRUE) {
            // Log queue is full, handle error as needed (e.g., drop message, set error flag)
        }
    }
}

/**
 * @brief Control devices based on sensor readings.
 * 
 * This function implements simple control logic:
 *  - If temperature > 25C, turn on AC and turn off heater.
 *  - If temperature < 20C, turn on heater and turn off AC.
 *  - If 20C <= temperature <= 25C, turn off both AC and heater.
 *  - If motion is detected, turn on light; otherwise, turn off light.
 * 
 * @param temperature Current temperature reading from the sensor.
 * @param motion Current motion reading from the sensor (0 or 1).
*/
static void control_devices(uint16_t temperature, uint16_t motion) 
{
    if (temperature > 25U) {
        turnOnAC();
        turnOffHeater();
        log_messages("Controller", "T > 25C : AC on, Heater off");
    } else if (temperature < 20U) {
        turnOnHeater();
        turnOffAC();
        log_messages("Controller", "T < 20C : Heater on, AC off");
    } else {
        turnOffAC();
        turnOffHeater();
        log_messages("Controller", "20C <= T <= 25C : AC off, Heater off");
    }

    if (motion > 0U) {
        turnOnLight();
        log_messages("Controller", "Motion detected: Light on");
    } else {
        turnOffLight();
        log_messages("Controller", "Motion not detected: Light off");
    }
}

/**
 * @brief Helper function to log messages for the control_devices function.
 * 
 * @param taskname Name of the task ie "Controller"
 * @param message  The control action taken ex. "Turned on AC, off heater"
*/
static void log_messages(const char* taskname, const char* message)
{
    char msg[LOG_MSG_MAX_LEN];
    BaseType_t xRet = pdFALSE;

    snprintf(msg, sizeof(msg), "[%-12s] %s", taskname, message);
    xRet = xQueueSend(xLogQueue, msg, 0U);
    if (xRet != pdTRUE) {
        /* Log queue full — increment error counter or set error flag */
    }
}