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

#include "tasks.h"
#include "wrapper.h"
#include "shared_resources.h"

volatile uint8_t task3_alive = 0U;

// Local function prototype
static void control_temp_devices(uint16_t temperature);
static void control_motion_devices(uint8_t motion);
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

    uint32_t   tick_count    = 0U; 
    char            msg[LOG_MSG_MAX_LEN];
    BaseType_t      xRet          = pdFALSE;
    SensorData_t    sensorData    = {0U};
    TXQueue_Item_t  queueItem      = {0U};

    while (1) 
    {
        // Block waiting for sensor data struct from Sensor Queue
        xRet = xQueueReceive(xSensorQueue, &sensorData, portMAX_DELAY);
        if (xRet != pdTRUE) {
            continue;
        }

        // Temp value available
        if (sensorData.flags & TEMP_SENSOR_DATA) {
            control_temp_devices(sensorData.tempData);

            queueItem.payloadType                  = TX_PAYLOAD_TEMP_DATA;
            queueItem.payload.tempData.temperature = sensorData.tempData;

            xRet = xQueueSend(xTXQueue, &queueItem, 0U);
            if (xRet != pdTRUE) {
                /* TX queue full — drop data */
            }
        }

        // Motion value available
        if (sensorData.flags & MOTION_SENSOR_DATA) {
            control_motion_devices(sensorData.motionData); 

            queueItem.payloadType               = TX_PAYLOAD_MOTION_DATA;
            queueItem.payload.motionData.motion = sensorData.motionData;

            xRet = xQueueSend(xTXQueue, &queueItem, 0U);
            if (xRet != pdTRUE) {
                /* TX queue full — drop data */
            }
        }

        if (tick_count++ % 100 == 0) {
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            LOG("Controller stack watermark: %u words remaining", watermark);
        }

        // Set alive flag
        task3_alive = 1;
        snprintf(msg, sizeof(msg), "[T3] Sent alive heartbeat");
        xRet = xQueueSend(xLogQueue, msg, 0U);
        if (xRet != pdTRUE) {
            /* Log queue full — increment error counter or set error flag */
        }
    }
}

/**
 * @brief Control temperature devices based on sensor readings.
 * 
 * This function implements simple control logic:
 *  - If temperature > 25C, turn on AC and turn off heater.
 *  - If temperature < 20C, turn on heater and turn off AC.
 *  - If 20C <= temperature <= 25C, turn off both AC and heater.
 * 
 * @param temperature Current temperature reading from the sensor.
*/
static void control_temp_devices(uint16_t temperature) 
{
    if (temperature > 25U) {
        turnOnAC();
        turnOffHeater();
       // log_messages("Controller", "T > 25C : AC on, Heater off");
    } else if (temperature < 20U) {
        turnOnHeater();
        turnOffAC();
      //  log_messages("Controller", "T < 20C : Heater on, AC off");
    } else {
        turnOffAC();
        turnOffHeater();
       // log_messages("Controller", "20C <= T <= 25C : AC off, Heater off");
    }
}

/**
 * @brief Control motion devices based on sensor readings.
 * 
 * This function implements simple control logic:
 *  - If motion is detected, turn on light; otherwise, turn off light.
 * 
 * @param motion Current motion reading from the sensor (0 or 1).
*/
static void control_motion_devices(uint8_t motion) 
{
    if (motion > 0U) {
        turnOnLight();
        //log_messages("Controller", "Motion detected: Light on");
    } else {
        turnOffLight();
        //log_messages("Controller", "Motion not detected: Light off");
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