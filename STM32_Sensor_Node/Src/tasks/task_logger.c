/**
 * @file task_logger.c
 * @brief System logger task.
 * 
 * Receives log messages from the log queue and prints them to UART2. 
 * This is the only task that writes to UART directly.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "uart.h"
#include "tasks.h"
#include "shared_resources.h"

void vTaskLogger(void *pvParameters)
{
    (void)pvParameters;                 // Suppress unused parameter warning

    char msg[LOG_MSG_MAX_LEN];
    BaseType_t ret;

    while (1) 
    {
        ret = xQueueReceive(xLogQueue, msg, portMAX_DELAY);
        if (ret == pdTRUE) {
            msg[LOG_MSG_MAX_LEN - 1] = '\0';  // Ensure null termination
            printf("%s\n\r", msg);
        }
    }
}

