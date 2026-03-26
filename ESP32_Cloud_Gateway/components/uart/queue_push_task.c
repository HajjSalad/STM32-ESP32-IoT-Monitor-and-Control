/**
 * @file  queue_push_task.c
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"
#include <stdint.h>
#include "stdio.h"

#include "uart_tasks.h"
#include "queue_push_task.h"
#include "shared_resources.h"

static const char *TAG = "Queue_PUSH";

static const TXQueue_Item_t commandItem = {
    .payloadType            = TX_PAYLOAD_COMMAND,
    .payload.command = {
        .code  = CMD_SET_TEMP_THRESHOLD,
        .value = 3000,
    },
};

static const TXQueue_Item_t firmwareItem = {
    .payloadType              = TX_PAYLOAD_FIRMWARE,
    .payload.firmware  = {
        .data         = {0x02},
        .chunk_index  = 0,
        .total_chunks = 1,
    },
};

static void queue_push_task(void *pvParameters)
{
    (void)pvParameters;

    uint32_t lastCommand  = 0;
    uint32_t lastFirmware = 0;

    while(1)
    {
        uint32_t now = xTaskGetTickCount();

        // Queue command every 5 seconds
        if ((now - lastCommand) >= pdMS_TO_TICKS(100000)) {
            lastCommand = now;
            if (xQueueSend(tx_queue, &commandItem, 0) == pdPASS) {
                //ESP_LOGI(TAG, "Command queued");
            } else {
               // ESP_LOGI(TAG, "tx_queue full, Command dropped");
            }
        }

        // Queue firmware every 10 seconds
        if ((now - lastFirmware) >= pdMS_TO_TICKS(100000)) {
            lastFirmware = now;
            if (xQueueSend(tx_queue, &firmwareItem, 0) == pdPASS) {
                //ESP_LOGI(TAG, "Firmware queued");
            } else {
               // ESP_LOGI(TAG, "tx_queue full, Firmware dropped");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

}

void queue_push_task_init(void)
{
    xTaskCreate(queue_push_task, "queue_push_task", 2048, NULL, TASK_PRIO_4, NULL);
}