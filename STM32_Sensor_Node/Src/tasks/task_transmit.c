/**
 * @file task_transmit.c
 * @brief
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "task_logger.h"
#include "task_transmit.h"
#include "shared_resources.h"

void vTaskTransmit(void *pvParameters)
{
    (void)pvParameters;                 // Suppress unused parameter warning

    char msg[LOG_MSG_MAX_LEN];
    BaseType_t      xRet;
    TransmitData_t  txData = {0U};

    while (1) 
    {
        // 1. Block waiting for transmit data struct from stream buffer
        xRet = xStreamBufferReceive(xStreamBuffer, &txData, sizeof(TransmitData_t), portMAX_DELAY);
        if (xRet != sizeof(TransmitData_t)) {
            // Handle error (e.g., log, continue, etc.)
            continue;
        }

        // 2. Send to ESP32 via UART
        // Future addition

        // 3. Log the transmitted sensor data
        LOG_TRANSMIT_DATA(msg, "Transmit", "Transmit to ESP32:", txData.temperature, txData.motion, txData.timestamp);
        xRet = xQueueSend(xLogQueue, msg, 0);
        if (xRet != pdTRUE) {
            // Log queue is full, handle error as needed (e.g., drop message, set error flag)
        }

        // Send blank line to separate data cycles
        snprintf(msg, sizeof(msg), " ");
        xRet = xQueueSend(xLogQueue, msg, 0);
        if (xRet != pdTRUE) {
            // Log queue is full
        }
    }
}