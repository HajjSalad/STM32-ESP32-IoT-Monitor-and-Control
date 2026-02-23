/**
 * @file uart_rxtx_task.c
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "string.h"
#include "stdio.h"

#include "uart.h"
#include "task_priorities.h"

static void rxtx_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t rx_data[RX_BUF_SIZE] = {0};

    while (1) {
        if (xQueueReceive(uart_2_queue, (void *)&event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                // Clear buffer before reading
                memset(rx_data, 0, sizeof(rx_data));
                
                // Read data (limit to buffer size - 1 to keep null-terminated)
                int to_read = (event.size < sizeof(rx_data)-1) ? event.size : sizeof(rx_data)-1;
                uart_read_bytes(UART_NUM2, rx_data, to_read, portMAX_DELAY);
                
                // Print received data
                printf("Received: %s\n", rx_data);

                // Handle protocol commands
                if (strncmp((char*)rx_data, "READY?", 6) == 0) {
                    // Send "OK" when STM32 sends "READY?"
                    const char *response_ok = "OK\n";
                    uart_write_bytes(UART_NUM2, response_ok, strlen(response_ok));
                    printf("Responded with: OK\n");
                } 
                else if (strncmp((char*)rx_data, "Message from STM32", 18) == 0) {
                    // Send "ACK" when STM32 sends message
                    const char *ack_response = "ACK\n";
                    uart_write_bytes(UART_NUM2, ack_response, strlen(ack_response));
                    printf("Responded with: ACK\n\n");
                }
                else {
                    printf("Unknown data received.\n\n");
                }
            }
        }
    }
    vTaskDelete(NULL);
}

void uart_rxtx_task_init(void)
{
    xTaskCreate(rxtx_task, "uart_rxtx_task", 2048, NULL, TASK_PRIO_CRITICAL, NULL);
}