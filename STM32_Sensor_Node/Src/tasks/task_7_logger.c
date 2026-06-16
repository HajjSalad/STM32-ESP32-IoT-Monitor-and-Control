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

#include "uart_driver.h"
#include "tasks.h"
#include "shared_resources.h"

volatile uint8_t task7_alive = 0U;

void vTaskLogger(void *pvParameters)
{
    (void)pvParameters;                 // Suppress unused parameter warning

    uint32_t   tick_count    = 0U; 
    char msg[LOG_MSG_MAX_LEN];
    BaseType_t xRet;

    while (1) 
    {
        xRet = xQueueReceive(xLogQueue, msg, portMAX_DELAY);
        if (xRet == pdTRUE) {
            msg[LOG_MSG_MAX_LEN - 1] = '\0';  // Ensure null termination
            printf("%s\n\r", msg);
        }

        // if (tick_count++ % 100 == 0) {
        //     UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        //     LOG("Logger stack watermark: %u words remaining", watermark);
        // }

        // // Set alive flag
        // if (msg_count % 100 == 0) {
        //     task7_alive = 1;
        // }
        // snprintf(msg, sizeof(msg), "[T7] Sent alive heartbeat");
        // xRet = xQueueSend(xLogQueue, msg, 0U);
        // if (xRet != pdTRUE) {
        //     /* Log queue full — increment error counter or set error flag */
        // }
    }
}

